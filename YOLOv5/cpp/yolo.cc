#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yolo.h"
#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"

#if defined(RV1106_1103) 
    #include "dma_alloc.hpp"
#endif

/* 模型 */
static rknn_app_context_t rknn_app_ctx;

/* yolo初始化 */
int yolo_init(char *model_path_tmp)
{
    const char *model_path = model_path_tmp;
    int ret;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    ret = init_post_process();
    if (ret != 0) {
        printf("init_post_process fail! ret=%d\n", ret);
        goto process_free;
    }
    ret = init_yolov5_model(model_path, &rknn_app_ctx);
    if (ret != 0)
    {
        printf("init_yolov5_model fail! ret=%d model_path=%s\n", ret, model_path);
        goto yolo_free;
    }

    return 0;

yolo_free:
    release_yolov5_model(&rknn_app_ctx);
process_free:
    deinit_post_process();
    return ret;
}

/* yolo检测 */
int yolo_detect(image_buffer_t* src_image)
{
    int ret = 0;

    object_detect_result_list od_results;

    ret = inference_yolov5_model(&rknn_app_ctx, src_image, &od_results);
    if (ret != 0)
    {
        printf("init_yolov5_model fail! ret=%d\n", ret);
        goto out;
    }

    // 画框和概率
    char text[256];
    for (int i = 0; i < od_results.count; i++)
    {
        object_detect_result *det_result = &(od_results.results[i]);
        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;

        draw_rectangle(src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);

        sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
        draw_text(src_image, text, x1, y1 - 20, COLOR_RED, 10);
    }

out:
    if (src_image->virt_addr != NULL)
    {
        // free(src_image->virt_addr);
    }
    return ret;
}
