#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QImage>
#include <QPixmap>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv/cv.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    QTimer theTimer;
    QLabel *imageLabel;
    QHBoxLayout layout;
    cv::VideoCapture cap;
    cv::Mat frame;

private slots:
    void updateImage();

protected:
    void paintEvent(QPaintEvent *e);
};
#endif // MAINWINDOW_H
