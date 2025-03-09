/**
 * OpenCv捕获数据，通过FFmpeg的编码器将数据发出
*/
#include "command.h"
#include "command_mpp.h"
#include "ffmpeg_with_mpp.h"
#include "encoder.h"


#include <rockchip/mpp_buffer.h>
#include <rockchip/rk_mpi.h>
#include <rockchip/mpp_frame.h>
#include <rockchip/mpp_meta.h>
#include <rockchip/mpp_packet.h>

#include <sys/time.h>

#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))


unsigned int framecount = 0;
unsigned int width = 0,height = 0;

unsigned int hor_stride = 0,ver_stride = 0; 

unsigned int yuv_width = 0,yuv_height = 0;
unsigned int yuv_hor_stride = 0,yuv_ver_stride = 0; 

unsigned int image_size = 0;

/*********************FFMPEG_START*/
const AVCodec * codec;
AVCodecContext *codecCtx;
AVFormatContext * formatCtx;
AVStream * stream;
AVHWDeviceType type = AV_HWDEVICE_TYPE_DRM;
AVBufferRef *hwdevice;
AVBufferRef *hwframe;
AVHWFramesContext * hwframeCtx;

AVFrame *frame; // 封装DRM的帧
AVPacket * packet; // 发送的包

long extra_data_size = 10000000;
uint8_t* cExtradata = NULL; // 数据头

AVPixelFormat hd_pix = AV_PIX_FMT_DRM_PRIME;
AVPixelFormat sw_pix = AV_PIX_FMT_RGB24;
/*********************FFMPEG_END*/
/**********************MPP_START*/
MppBufferGroup group;
MppBufferInfo info;
MppBuffer buffer;
MppBuffer commitBuffer;
MppFrame mppframe;
MppPacket mppPacket;

MppApi *mppApi;
MppCtx mppCtx;
MppEncCfg cfg;
MppTask task;
MppMeta meta;

typedef struct {
    MppPacket packet;
    AVBufferRef *encoder_ref;
} RKMPPPacketContext;
/************************MPP_END*/

void rkmpp_release_packet(void *opaque, uint8_t *data){
    RKMPPPacketContext *pkt_ctx = (RKMPPPacketContext *)opaque;
    mpp_packet_deinit(&pkt_ctx->packet);
    av_buffer_unref(&pkt_ctx->encoder_ref);
    av_free(pkt_ctx);
}

int init_encoder_ffmpeg(Command & obj){
    int res = 0;
    avformat_network_init();

    codec = avcodec_find_encoder_by_name("h264_rkmpp");
    if(!codec){
        print_error(__LINE__,-1,"can not find h264_rkmpp encoder!");
        return -1;
    }
    // 创建编码器上下文
    codecCtx = avcodec_alloc_context3(codec);
    if(!codecCtx){
        print_error(__LINE__,-1,"can not create codec Context of h264_rkmpp!");
        return -1;
    }
    
    res = av_hwdevice_ctx_create(&hwdevice,type,"/dev/dri/card0",0,0);
    if(res < 0){
        print_error(__LINE__,res,"create hdwave device context failed!");
        return res;
    }

    codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    codecCtx->codec_id = codec->id;
    codecCtx->codec = codec;
    codecCtx->bit_rate = 1024*1024*8;
    codecCtx->codec_type = AVMEDIA_TYPE_VIDEO; //解码类型: 视频
    codecCtx->width = width;  // 宽
    codecCtx->height = height; // 高
    codecCtx->channels = 0;     // 非音频通道
    codecCtx->time_base = (AVRational){1,obj.get_fps()}; // 每帧的时间：1/obj.get_fps()
    codecCtx->framerate = (AVRational){obj.get_fps(),1}; // 帧率： obj.get_fps()/1
    codecCtx->pix_fmt = hd_pix; //AV_PIX_FMT_DRM_PRIME
    codecCtx->gop_size = 25; // 每组多少帧
    codecCtx->max_b_frames = 0; // b帧最大间隔

    // 设置编码器的硬件上下文
    hwframe = av_hwframe_ctx_alloc(hwdevice);
    if(!hwframe){
        print_error(__LINE__,-1,"create hdwave frame context failed!");
        return -1;
    }
    hwframeCtx = (AVHWFramesContext *)(hwframe->data);
    hwframeCtx->format    = hd_pix;                     // 硬件帧格式设置为DRM
    hwframeCtx->sw_format = sw_pix;                     // 软件帧格式设置为RGB24
    hwframeCtx->width     = width;
    hwframeCtx->height    = height;
    /**
     *  帧池，会预分配，后面创建与硬件关联的帧时，会从该池后面获取相应的帧
     *  initial_pool_size与pool 至少要有一个不为空
    */
    // hwframeCtx->initial_pool_size = 20;
    hwframeCtx->pool = av_buffer_pool_init(20*sizeof(AVFrame),NULL);

    // 初始化硬件帧上下文
    res = av_hwframe_ctx_init(hwframe);
    if(res < 0){
        print_error(__LINE__,res,"init hd frame context failed!");
        return res;
    }
    codecCtx->hw_frames_ctx = hwframe;
    codecCtx->hw_device_ctx = hwdevice;

    // 初始化输出流格式结构体
    if(!strcmp(obj.get_protocol(), "rtsp")){
        // rtsp协议
        res = avformat_alloc_output_context2(&formatCtx,NULL,"rtsp",obj.get_url());
    }else{
        // rtmp协议
        res = avformat_alloc_output_context2(&formatCtx,NULL,"flv",obj.get_url());
    }
    if(res < 0){
        print_error(__LINE__,res,"create output context failed!");
        return res;
    }

    // 创建输出流
    stream = avformat_new_stream(formatCtx,codec);
    if(!stream){
        print_error(__LINE__,res,"create stream failed!");
        return -1;
    }
    stream->time_base = (AVRational){1,obj.get_fps()}; // 设置帧率
    stream->id = formatCtx->nb_streams - 1; // 设置流的索引

    // 复制参数到流
    res = avcodec_parameters_from_context(stream->codecpar,codecCtx);
    if(res < 0){
        print_error(__LINE__,res,"copy parameters to stream failed!");
        return -1;
    }

    // 打开输出IO RTSP不需要打开，RTMP需要打开
    if(!strcmp(obj.get_protocol(),"rtmp")){
        res = avio_open2(&formatCtx->pb,obj.get_url(),AVIO_FLAG_WRITE,NULL,NULL);
        if(res < 0){
            print_error(__LINE__,res,"avio open failed !");
            return -1;
        }
    }
    // 写入头信息   
    AVDictionary *opt = NULL;
    if(!strcmp(obj.get_protocol(),"rtsp")){
        // rtsp协议
        av_dict_set(&opt, "rtsp_transport",obj.get_trans_protocol(),0);
        // 多路复用延迟时间
        av_dict_set(&opt, "muxdelay", "0.1", 0);
    }
	res = avformat_write_header(formatCtx, &opt);
	if(res < 0){
        print_error(__LINE__,res,"avformat write header failed ! ");
        return -1;
	}
    av_dump_format(formatCtx, 0, obj.get_url(), 1);
    return res;
}

MPP_RET init_mpp(){
    MPP_RET res = MPP_OK;
    // 创建 MPP 上下文 mppCtx 并获取 MPP API 接口 mppApi
    res = mpp_create(&mppCtx,&mppApi);
    // 初始化 MPP 上下文，指定为编码器上下文（MPP_CTX_ENC），并使用 AVC（H.264）编码（MPP_VIDEO_CodingAVC）
    res = mpp_init(mppCtx,MPP_CTX_ENC,MPP_VIDEO_CodingAVC);
    // 初始化编码器配置结构体 cfg
    res = mpp_enc_cfg_init(&cfg);
    // 获取编码器默认配置
    res = mppApi->control(mppCtx,MPP_ENC_GET_CFG,cfg);

    mpp_enc_cfg_set_s32(cfg, "prep:width", width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", height);
    // 设置水平步长
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", hor_stride * 3);
    // 设置垂直步长
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_RGB888);

    mpp_enc_cfg_set_s32(cfg, "rc:quality", MPP_ENC_RC_QUALITY_BEST);
    // 编码模式为可变比特率
    mpp_enc_cfg_set_s32(cfg, "rc:mode", MPP_ENC_RC_MODE_VBR);

    /* fix input / output frame rate */
    // 输入帧率不变
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", 0);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", 30);      // 60fps
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm",1);
    // 输出帧率不变
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", 0);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", 30);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", 1);

    /* drop frame or not when bitrate overflow */
    // 比特率溢出时不丢帧
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_max", 8*1024*1024 * 17 / 16);
    mpp_enc_cfg_set_s32(cfg, "rc:bps_min", 8*1024*1024 * 15 / 16);

    mpp_enc_cfg_set_s32(cfg,"split:mode", MPP_ENC_SPLIT_NONE);
    // mpp_enc_cfg_set_s32(cfg,"split_arg", 0);
    // mpp_enc_cfg_set_s32(cfg,"split_out", 0);

    mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
    mpp_enc_cfg_set_s32(cfg, "h264:level", 42);
    mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
    mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
    mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

    // 重新配置编码器
    mppApi->control(mppCtx, MPP_ENC_SET_CFG, cfg);

    // 创建缓冲区
    MppPacket headpacket;
    RK_U8 enc_hdr_buf[1024];
    memset(enc_hdr_buf,0,1024);

    // 获取头数据
    mpp_packet_init(&headpacket,enc_hdr_buf,1024);

    res = mppApi->control(mppCtx, MPP_ENC_GET_HDR_SYNC, headpacket);
    void *ptr   = mpp_packet_get_pos(headpacket);
    size_t len  = mpp_packet_get_length(headpacket);

    extra_data_size = len;
    cExtradata = (uint8_t *)malloc((extra_data_size) * sizeof(uint8_t));

    memcpy(cExtradata,ptr,len); // 拷贝头数据
    mpp_buffer_group_get_external(&group,MPP_BUFFER_TYPE_DRM);
    mpp_packet_deinit(&headpacket);

    cout << __LINE__ << " init mpp encoder  finished !" << endl;
    return res;
}

int init_data(Command & obj){
    int res = 0;
    // 给packet 分配内存
    packet = av_packet_alloc();
    return res;
}

int send_packet(Command &obj){
    int res = 0;
    packet->pts = av_rescale_q_rnd(framecount, codecCtx->time_base, stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
    packet->dts = av_rescale_q_rnd(framecount, codecCtx->time_base, stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
    packet->duration = av_rescale_q_rnd(packet->duration, codecCtx->time_base, stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));

    if(!(packet->flags & AV_PKT_FLAG_KEY)){
        // 在每帧非关键帧前面添加PPS SPS头信息
        /**
         * 使用h264_rkmpp编码器时，rtsp/rtmp协议都需要添加PPS
         * libx264只需要在rtsp协议时添加PPS,rtmp会自动加上
        */
        int packet_data_size = packet->size;
        u_char frame_data[packet_data_size];
        memcpy(frame_data,packet->data,packet->size);
        memcpy(packet->data,cExtradata,extra_data_size);
        memcpy(packet->data+extra_data_size,frame_data,packet_data_size);
        packet->size = packet_data_size + extra_data_size;
    }
    // 通过创建输出流的format 输出数据包
    framecount++;
    res = av_interleaved_write_frame(formatCtx, packet);
    if (res < 0){
        print_error(__LINE__,res,"send packet error!");
        return -1;
    }
    return 0;
}
/**
 * 将opencv的帧转换成drm数据帧
 * 并送入mpi解码，转换成AvPacket
*/
MPP_RET convert_cvframe_to_drm(cv::Mat &cvframe,AVFrame *& avframe,Command & obj){
    MPP_RET res = MPP_OK;
    res = mpp_buffer_get(NULL,&buffer,image_size);
    if(res != MPP_OK){
        return res;
    }
    info.fd = mpp_buffer_get_fd(buffer);
    info.ptr = mpp_buffer_get_ptr(buffer);
    info.index = framecount;
    info.size = image_size;
    info.type = MPP_BUFFER_TYPE_DRM;

    memcpy(info.ptr,cvframe.datastart,height * width * 3);

    res = mpp_buffer_commit(group,&info);
    if(res != MPP_OK){
        return res;
    }

    res = mpp_buffer_get(group,&commitBuffer,image_size);
    if(res != MPP_OK){
        return res;
    }

    mpp_frame_init(&mppframe);
    mpp_frame_set_width(mppframe,width);
    mpp_frame_set_height(mppframe,height);
    mpp_frame_set_hor_stride(mppframe,hor_stride * 3);
    mpp_frame_set_ver_stride(mppframe,ver_stride);
    mpp_frame_set_buf_size(mppframe,image_size);
    mpp_frame_set_buffer(mppframe,commitBuffer);
    /**
     * 使用mpp可以使用 YUV格式的数据外 还能使用RGB格式的数据
    */
    mpp_frame_set_fmt(mppframe,MPP_FMT_RGB888); // YUV420SP == NV12 
    mpp_frame_set_eos(mppframe,0);

    mpp_packet_init_with_buffer(&mppPacket, commitBuffer);
    mpp_packet_set_length(mppPacket, 0);
    // set frame
    // 对输入端口进行轮询，从输入端口出队一个任务，将 mppPacket 和 mppframe 设置到任务的元数据中，然后将任务重新入队到输入端口
    res = mppApi->poll(mppCtx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
    res = mppApi->dequeue(mppCtx, MPP_PORT_INPUT, &task);
    res = mpp_task_meta_set_packet(task,KEY_OUTPUT_PACKET,mppPacket);       // ????
    res = mpp_task_meta_set_frame(task, KEY_INPUT_FRAME, mppframe);
    res = mppApi->enqueue(mppCtx, MPP_PORT_INPUT, task);

    // get packet
    // 对输出端口进行轮询，从输出端口出队一个任务，获取任务元数据中的数据包，然后将任务重新入队到输出端口。
    res = mppApi->poll(mppCtx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
    res = mppApi->dequeue(mppCtx, MPP_PORT_OUTPUT, &task);
    res = mpp_task_meta_get_packet(task, KEY_OUTPUT_PACKET, &mppPacket);
    res = mppApi->enqueue(mppCtx, MPP_PORT_OUTPUT, task);

    int is_eoi = mpp_packet_is_eoi(mppPacket);

    const char * temp = ("create time :" +to_string(av_gettime())).c_str();
    if (mppPacket) {
        RKMPPPacketContext *pkt_ctx = (RKMPPPacketContext *)av_mallocz(sizeof(*pkt_ctx));
        pkt_ctx->packet = mppPacket;
        int keyframe = 0;
        // TODO: outside need fd from mppbuffer?
        packet->data = (uint8_t *)mpp_packet_get_data(mppPacket);
        packet->size = mpp_packet_get_length(mppPacket);

        packet->buf = av_buffer_create((uint8_t*)packet->data, packet->size,
            rkmpp_release_packet, pkt_ctx, AV_BUFFER_FLAG_READONLY);
        packet->pts = mpp_packet_get_pts(mppPacket);
        packet->dts = mpp_packet_get_dts(mppPacket);
        packet->opaque = (void *)temp;
        if (packet->pts <= 0)
            packet->pts = packet->dts;
        if (packet->dts <= 0)
            packet->dts = packet->pts;
        meta = mpp_packet_get_meta(mppPacket);
        if (meta)
            mpp_meta_get_s32(meta, KEY_OUTPUT_INTRA, &keyframe);
        if (keyframe){
            packet->flags |= AV_PKT_FLAG_KEY;
        }
    }

    send_packet(obj);
    return res;
}

int transfer_frame(cv::Mat &cvframe,Command &obj){
    int res = 0;
    
    convert_cvframe_to_drm(cvframe,frame,obj);

    if(buffer != NULL){
        mpp_buffer_put(buffer); // 清空buffer
        buffer = NULL;
    }
    if(commitBuffer != NULL){
        mpp_buffer_put(commitBuffer); // 清空buffer
        commitBuffer = NULL;
    }
    /**
     * TODO:为什么这里不需要deinit
     * 还是说编码完 对引用的buffer自动释放？
    */
    // mpp_packet_deinit(&mppPacket); 
    mpp_buffer_group_clear(group);
    mpp_frame_deinit(&mppframe);
    return 0;
}

void destory_(){
    cout << "释放回收资源：" << endl;
    mpp_buffer_group_put(group);
	// fclose(wf);
	if(formatCtx){
        // avformat_close_input(&avFormatCtx);
        avio_close(formatCtx->pb);
        avformat_free_context(formatCtx);
        formatCtx = 0;
        cout << "avformat_free_context(formatCtx)" << endl;
    }
    if(packet){
        av_packet_unref(packet);
        packet = NULL;
        cout << "av_free_packet(packet)" << endl;
    }

    if(frame){
        av_frame_free(&frame);
        frame = 0;
        cout << "av_frame_free(frame)" << endl;
    }
   
    if(codecCtx->hw_device_ctx){
        av_buffer_unref(&codecCtx->hw_device_ctx);
        cout << "av_buffer_unref(&codecCtx->hw_device_ctx)" << endl;
    }

    if(codecCtx){
        avcodec_close(codecCtx);
        codecCtx = 0;
        cout << "avcodec_close(codecCtx);" << endl;
    }
    if(cExtradata){
        free(cExtradata);
        cout << "free cExtradata " << endl;
    }
    if(hwdevice){
        av_buffer_unref(&hwdevice);
        cout << "av_buffer_unref hwdevice " << endl;
    }
    if(hwframe){
        av_buffer_unref(&hwframe);
        cout << "av_buffer_unref encodeHwBufRef " << endl;
    }
}

static int is_init_encoder = 0;
Command obj;
int encode_push(Mat cvframe)
{
    int res = 0;
    auto now = av_gettime();
    auto convert = av_gettime();
    if (!is_init_encoder) {
        Size size = cvframe.size();
        obj = process_command();
        width = size.width;
        height = size.height;
        
        hor_stride = MPP_ALIGN(width, 16); 
        ver_stride = MPP_ALIGN(height, 16);

        image_size = sizeof(unsigned char) * hor_stride *  ver_stride * 3;
        cout << " width " << width << endl;
        cout << " height " << height << endl;
        cout << " hor_stride " << hor_stride << endl;
        cout << " ver_stride " << ver_stride << endl;
        res = init_encoder_ffmpeg(obj); //初始化解码器
        if(res < 0){
            print_error(__LINE__,res,"init encoder failed!");
            return -1;
        }
        res = init_data(obj);
        if(res < 0){
            print_error(__LINE__,res,"init data failed!");
            return -1;
        }
        res = init_mpp();
        if(res < 0){
            print_error(__LINE__,res,"init mpp failed!");
            return -1;
        }
        is_init_encoder = 1;
    }
    transfer_frame(cvframe, obj);

    auto end = av_gettime();
    struct timeval start;

    gettimeofday(&start,NULL); //gettimeofday(&start,&tz);结果一样
    // cout << framecount << ": ";
    // cout << "["<< start.tv_sec << " :" << start.tv_usec <<"]send frame use time [ "<< (end - now) / 1000 <<" ms]";
    // cout << " encoder use time [ "<< (end - convert) / 1000 <<" ms]" << endl;

    av_packet_unref(packet);
    return 0;
}