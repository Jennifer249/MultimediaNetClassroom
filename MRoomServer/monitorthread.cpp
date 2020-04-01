#include "monitorthread.h"

MonitorThread::MonitorThread(QString userName, QObject *parent)
    :QObject(parent)
{
   buf = new char[1024*1024];
   memset(buf, 0, 1024);
   this->userName = userName;
   pCount = 1;
}

MonitorThread::~MonitorThread()
{
    delete buf;
}

void MonitorThread::recvMonitor(char *imageBuf)
{
    qDebug() << "recv ok";
    ImageFrameHead *frame = (ImageFrameHead *)imageBuf;
    qDebug() << "pCount" << pCount <<frame->uDataFrameCurr;
    if(pCount != (int)frame->uDataFrameCurr)
    {
        memset(buf, 0, 1024);
        pCount = 1;
        return;
    }

    if(frame->funCode == 1)
    {
        pCount++;
        memcpy(buf+frame->uDataInFrameOffset, (imageBuf+sizeof(ImageFrameHead)), frame->uTransFrameCurrSize);
        if(frame->uDataFrameCurr == frame->uDataFrametotal)
        {
            pCount = 1;
            qDebug() << "==ok" <<userName;
            emit recvMonitorOk(userName, buf, frame->uDataFrameSize);
//            memset(buf, 0, 1024);
        }
    }
}

QString MonitorThread::getUserName()
{
    return userName;
}
