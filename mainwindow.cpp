#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&theTimer, &QTimer::timeout, this, &MainWindow::updateImage);
    if (!cap.open("/dev/video9"))
    {
//        QDebug() << "Error: Cannot open the video file" << endl;
        cap.release();
        return;
    }
    frame = cv::Mat::zeros(cap.get(CV_CAP_PROP_FRAME_HEIGHT), cap.get(CV_CAP_PROP_FRAME_WIDTH), CV_8UC3);
    theTimer.start(33);
    imageLabel = new QLabel(this);
    layout.addWidget(imageLabel);
    // ui->centralwidget(imageLabel);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *e)
{
    QImage image2 = QImage((uchar*)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888);
    imageLabel->setPixmap(QPixmap::fromImage(image2));
    imageLabel->resize(image2.size());
    imageLabel->show();
}

void MainWindow::updateImage()
{
    cap >> frame;
    if (frame.data) {
        cv::cvtColor(frame, frame, CV_BGR2RGB);
        this->update();
    }
}

