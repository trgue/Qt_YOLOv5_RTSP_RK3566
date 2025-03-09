#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QImage>
#include <QPixmap>
#include <QUdpSocket>
#include <QBuffer>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv/cv.h>
#include "yolo.h"
#include "encoder.h"

#if 0
#define QT_SHOW
#endif


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr, char *model_path_tmp = nullptr);
    ~MainWindow();
    void frame_to_image_buffer(cv::Mat* frame, image_buffer_t* src_image);
    void image_buffer_to_frame(cv::Mat* frame, image_buffer_t* src_image);
    void video_send(QImage img);
    QImage image2;
    image_buffer_t *src_image;
private:
    Ui::MainWindow *ui;
    QTimer theTimer;
    QUdpSocket *udpSocket;
    cv::VideoCapture cap;
    cv::Mat frame;
    cv::Mat detectedFrame;



private slots:
    void updateImage();

protected:
    void paintEvent(QPaintEvent *e);
};
#endif // MAINWINDOW_H
