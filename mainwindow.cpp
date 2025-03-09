#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <opencv2/videoio/videoio_c.h>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent, char *model_path_tmp)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&theTimer, &QTimer::timeout, this, &MainWindow::updateImage);
    if (yolo_init(model_path_tmp))
    {
        qDebug() << "Error: yolo_init fail" << endl;
        return;
    }
    /* 摄像头设备 */
    if (!cap.open("/dev/video9"))
    {
        qDebug() << "Error: Cannot open the video file" << endl;
        cap.release();
        return;
    }
#ifdef QT_SHOW
    /* UDP套接字 */
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::Any, 8888);
#endif
    /* 定义帧 */
    frame = cv::Mat::zeros(cap.get(CV_CAP_PROP_FRAME_HEIGHT), cap.get(CV_CAP_PROP_FRAME_WIDTH), CV_8UC3);
    src_image = new image_buffer_t;
    memset(src_image, 0, sizeof(image_buffer_t));
    theTimer.start(30);

}

MainWindow::~MainWindow()
{
    if (src_image->virt_addr != NULL)
    {
        free(src_image->virt_addr);
    }
    delete ui;
}

/* 将检测结果画到label上 */
void MainWindow::paintEvent(QPaintEvent *e)
{
#ifdef QT_SHOW
    ui->labeldisplay->setPixmap(QPixmap::fromImage(image2));
    ui->labeldisplay->resize(image2.size());
    ui->labeldisplay->show();
#endif
}

/* 检测更新 */
void MainWindow::updateImage()
{
    cap >> frame;
    if (frame.data) {
        clock_t start = clock();
        cv::cvtColor(frame, frame, CV_BGR2RGB);
        frame_to_image_buffer(&frame, src_image);
        yolo_detect(src_image);
        clock_t detect_end = clock();
        image_buffer_to_frame(&frame, src_image);
        encode_push(frame);
        clock_t encode_end = clock();
        qDebug() << "detect time: " << (double)(detect_end - start)/CLOCKS_PER_SEC;
        qDebug() << "encode time: " << (double)(encode_end - detect_end)/CLOCKS_PER_SEC;
        qDebug() << "total time: " << (double)(encode_end - start)/CLOCKS_PER_SEC << endl;
#ifdef QT_SHOW        
        image2 = QImage((uchar*)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888);
        video_send(image2);
#endif
        this->update();
    }
}

/* 帧转换成buffer */
void MainWindow::frame_to_image_buffer(cv::Mat* frame, image_buffer_t* src_image)
{
    src_image->width = frame->cols;
    src_image->height = frame->rows;
    unsigned char* pixeldata = new unsigned char[frame->cols * frame->rows * frame->channels()];
    memcpy(pixeldata, frame->data, frame->cols * frame->rows * frame->channels());
    if (src_image->virt_addr != NULL) {
        memcpy(src_image->virt_addr, pixeldata, frame->cols * frame->rows * frame->channels());
        free(pixeldata);
    } else {
        src_image->virt_addr = pixeldata;
    }
    src_image->format = IMAGE_FORMAT_RGB888;
}

/* buffer转换成帧 */
void MainWindow::image_buffer_to_frame(cv::Mat* frame, image_buffer_t* src_image)
{
    if (src_image->virt_addr != NULL) {
        *frame = cv::Mat(src_image->height, src_image->width, CV_8UC3, src_image->virt_addr);
    }
}

/* UDP发送，这里的目标ip和目标端口要和目标主机对应 */
void MainWindow::video_send(QImage img)
{
    QHostAddress ip = (QHostAddress)("192.168.137.1");
    QByteArray byte;
    QBuffer buff(&byte);
    buff.open(QIODevice::WriteOnly);
    img.save(&buff,"JPEG");
    QByteArray ss = qCompress(byte,5);
    udpSocket->writeDatagram(ss,ip,443);
}
