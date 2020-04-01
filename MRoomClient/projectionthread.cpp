#include "projectionthread.h"
#include "header.h"

ProjectionThread::ProjectionThread(QObject *parent) : QObject(parent)
{
    buf = new char[1024*1024];
    memset(buf,0,1024);
    pCount = 1;
}

ProjectionThread::~ProjectionThread()
{
    delete buf;
}

void ProjectionThread::recvProjection(char *imageBuf)
{
    qDebug() << "recv ok";
    ImageFrameHead *frame = (ImageFrameHead *)imageBuf;

    qDebug() << "pCount" << pCount << frame->uDataFrameCurr;
    if(pCount != (int)frame->uDataFrameCurr)
    {
        memset(buf, 0, 1024);
        pCount = 1;
        emit lostPacket(pCount);
        return;
    }
    if(frame->funCode == 0)
    {
        pCount++;
        memcpy(buf+frame->uDataInFrameOffset, (imageBuf+sizeof(ImageFrameHead)), frame->uTransFrameCurrSize);
        if(frame->uDataFrameCurr == frame->uDataFrametotal)
        {
            pCount = 1;
            emit recvProjectionOk(buf, frame->uDataFrameSize);
        }
    }
}
