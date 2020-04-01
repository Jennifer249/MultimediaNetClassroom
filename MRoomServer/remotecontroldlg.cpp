#include "remotecontroldlg.h"
#include "ui_remotecontroldlg.h"
#include <QPainter>
RemoteControlDlg::RemoteControlDlg(QString userName, QString userIP, quint16 controlPort, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RemoteControlDlg)
{
    ui->setupUi(this);
    setWindowTitle(tr("学生控制：")+userName);
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    this->userName = userName;
    this->userIP = userIP;
    this->controlPort = controlPort;
    this->setMouseTracking(true);
}

RemoteControlDlg::~RemoteControlDlg()
{
    delete ui;
}
//背景绘图
void RemoteControlDlg::paintEvent(QPaintEvent *event)
{
//    Q_UNUSED(event);

//    //设置背景色
//    QPalette palette(this->palette());
//    palette.setColor(QPalette::Background, Qt::black);
//    this->setPalette(palette);
//    this->setAutoFillBackground(true);
//    this->setPalette(palette);

    QPainter painter(this);
    painter.drawPixmap( 0, 0, this->width(), this->height(),pixmap);
}

void RemoteControlDlg::resizeEvent(QResizeEvent *event)
{
    event->accept();
    QString winwidth;
    winwidth.setNum(this->width());
    QString winheight;
    winheight.setNum(this->height());
    sendRemoteMsg("WINSIZE",winwidth,winheight);
}

void RemoteControlDlg::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    QString xx;
    xx.setNum(event->windowPos().x());
    QString yy;
    yy.setNum(event->windowPos().y());
    if(event->button()==Qt::LeftButton)
    {
        sendRemoteMsg("POI_LPRESS",xx,yy);
    }
    if(event->button()==Qt::RightButton)
    {
        sendRemoteMsg("POI_RPRESS",xx,yy);
    }
}
void RemoteControlDlg::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    QString xx;
    xx.setNum(event->windowPos().x());
    QString yy;
    yy.setNum(event->windowPos().y());
    sendRemoteMsg("POI_MOVE",xx,yy);
}

void RemoteControlDlg::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    QString xx;
    xx.setNum(event->windowPos().x());
    QString yy;
    yy.setNum(event->windowPos().y());
    if(event->button()==Qt::LeftButton)
    {
        sendRemoteMsg("POI_LRELEASE",xx,yy);
    }
    if(event->button()==Qt::RightButton)
    {
        sendRemoteMsg("POI_RRELEASE",xx,yy);
    }
}

void RemoteControlDlg::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();
    QString xx;
    xx.setNum(event->windowPos().x());
    QString yy;
    yy.setNum(event->windowPos().y());
    sendRemoteMsg("POI_DCLICK",xx,yy);
}

void RemoteControlDlg::keyPressEvent(QKeyEvent *event)
{
    event->accept();
    QString keynum;
    keynum.setNum(event->key());
    sendRemoteMsg("KEY_PRESS",keynum,event->text());
}

void RemoteControlDlg::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
    QString keynum;
    keynum.setNum(event->key());
    sendRemoteMsg("KEY_RELEASE",keynum,event->text());
}

void RemoteControlDlg::sendRemoteMsg(QString mouseType, QString msga, QString msgb)
{
    QString name = "Teacher";
    QString type = "CON_REMOTEMSG";
    QByteArray sData;
    QDataStream out(&sData, QIODevice::WriteOnly);
    out << name;
    out << type;
    out << mouseType;
    out << msga;
    out << msgb;
    qDebug() << "mouseType:"<<mouseType<< msga << msgb <<userIP;
    udpSocket->writeDatagram(sData, sData.length(), QHostAddress(userIP), controlPort);
}

void RemoteControlDlg::setSocket(QUdpSocket * udpSocket)
{
    this->udpSocket = udpSocket;
}

void RemoteControlDlg::showRemoteControl(char *buf, int length)
{
    qDebug() << "here";
    pixmap.loadFromData((uchar*)buf, length, "JPG");
    this->update();
//    int width = ui->remoteControlLabel->width()*0.9;
//    int height = ui->remoteControlLabel->height()*0.9;
//    QPixmap fitPixmap = pixmap.scaled(width,height,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
//    ui->remoteControlLabel->setPixmap(fitPixmap);
//    ui->remoteControlLabel->setAlignment(Qt::AlignCenter);

//    QPainter painter(this);
//    painter.drawPixmap( 0, 0, this->width(), this->height(),pixmap);
}

void RemoteControlDlg::closeEvent(QCloseEvent *event)
{
    QString name = "Teacher";
    QString type = "CON_STOP_REMOTECONTROL";
    QByteArray sData;
    QDataStream out(&sData, QIODevice::WriteOnly);
    out << name;
    out << type;
    udpSocket->writeDatagram(sData, sData.length(), QHostAddress(userIP), controlPort);
}
