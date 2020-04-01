#include "mserverthread.h"
#include <QDebug>
MServerThread::MServerThread(char userName[], QObject *parent)
    :QThread(parent)
{
   buf = new char[1024*1024];
   memset(buf, 0, 1024);
   this->userName = userName;
   cou = 0;
}

MServerThread::~MServerThread()
{
    udpSocket->close();
    delete buf;
}

void MServerThread::run()
{
}

void MServerThread::recvMonitor(char *imageBuf)
{
    //qDebug() << "recv ok";
    ImageFrameHead *frame = (ImageFrameHead *)imageBuf;
    if(frame->funCode == 1)
    {
        memcpy(buf+frame->uDataInFrameOffset, (imageBuf+sizeof(ImageFrameHead)), frame->uTransFrameCurrSize);
       // qDebug()<<"frame->uDataFrameCurr"<<frame->uDataFrameCurr;
        if(frame->uDataFrameCurr == frame->uDataFrametotal)
        {
            qDebug()<<"cou:"<<cou++;
            qDebug()<<"frame->uDataFrametotal"<<frame->uDataFrametotal;
            emit recvMonitorOk(userName, buf, frame->uDataFrameSize);
            memset(buf, 0, 1024);
        }
    }
}

char* MServerThread:: getUserName()
{
    return userName;
}
