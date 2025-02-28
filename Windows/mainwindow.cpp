#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QHostAddress>
#include<QPixmap>
#include<QImageReader>
#include<QBuffer>
#include<QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /* 绑定443端口，需要和发送端对应 */
    rci.bind(QHostAddress::Any, 443);
    connect(&rci, SIGNAL(readyRead()), this, SLOT(video_receive_show()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::video_receive_show()
{
    int size = rci.pendingDatagramSize();
    QByteArray buff;
    buff.resize(size);
    QHostAddress adrr ;
    quint16 port;
    rci.readDatagram(buff.data(),buff.size(),&adrr,&port);
    buff = qUncompress(buff);
    QBuffer buffer(&buff);
    QImageReader reader(&buffer,"JPEG");
    QImage image = reader.read();
    ui->label->setPixmap(QPixmap::fromImage(image));
    ui->label->resize(image.width(),image.height());
    ui->label->show();
}
