#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent, char *model_path_tmp)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QWidget *parentWidget = new QWidget();
    connect(&theTimer, &QTimer::timeout, this, &MainWindow::updateImage);
    if (yolo_init(model_path_tmp))
    {
        qDebug() << "Error: yolo_init fail" << endl;
        return;
    }
    if (!cap.open("/dev/video9"))
    {
        qDebug() << "Error: Cannot open the video file" << endl;
        cap.release();
        return;
    }
    /* 定义帧 */
    frame = cv::Mat::zeros(cap.get(CV_CAP_PROP_FRAME_HEIGHT), cap.get(CV_CAP_PROP_FRAME_WIDTH), CV_8UC3);
    theTimer.start(33);

    /* 设置显示界面 */
    imageLabel = new QLabel(this);
    layout.addStretch();
    layout.addWidget(imageLabel);
    layout.addStretch();
    parentWidget->setStyleSheet("background-color:black;");
    parentWidget->setMinimumSize(900, 600);
    parentWidget->setLayout(&layout);
    setCentralWidget(parentWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* 将检测结果画到label上 */
void MainWindow::paintEvent(QPaintEvent *e)
{
    QImage image2 = QImage((uchar*)(detectedFrame.data), detectedFrame.cols, detectedFrame.rows, QImage::Format_RGB888);
    imageLabel->setPixmap(QPixmap::fromImage(image2));
    imageLabel->resize(image2.size());
    imageLabel->show();
}

/* 检测更新 */
void MainWindow::updateImage()
{
    cap >> frame;
    if (frame.data) {
        cv::cvtColor(frame, frame, CV_BGR2RGB);
        image_buffer_t src_image;
        memset(&src_image, 0, sizeof(image_buffer_t));
        frame_to_image_buffer(&frame, &src_image);
        yolo_detect(&src_image);
        detectedFrame = image_buffer_to_frame(&src_image);
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
cv::Mat MainWindow::image_buffer_to_frame(image_buffer_t* src_image)
{
    cv::Mat tmp;
    if (src_image->virt_addr != NULL) {
        tmp = cv::Mat(src_image->height, src_image->width, CV_8UC3, src_image->virt_addr);
    }
    return tmp;
}
