#include "udatasocket.h"
#include "header.h"
#include <QNetworkInterface>
UDataSocket::UDataSocket(quint16 port, QNetworkInterface intf, QString cIP, QObject *parent)
    :QThread(parent)
{
   buf = new char[1024*1024*100];
   memset(buf, 0, 1024);
   dataPort = port;
   pCount = 1;
   this->intf = intf;
   this->cIP = cIP;
   lastCount = 0;
}

UDataSocket::~UDataSocket()
{
    uDataSocket->close();
    delete buf;
}

void UDataSocket::run()
{
//    QThread thread;
//    projectionThread = new ProjectionThread();
//    projectionThread->moveToThread(&thread);

//    connect(projectionThread, SIGNAL(recvProjectionOk(char*,int)), this, SIGNAL(recvProjectionOk(char*,int)),Qt::DirectConnection);
//    connect(projectionThread, SIGNAL(lostPacket(int)), this, SIGNAL(lostPacket(int)),Qt::DirectConnection);

    uDataSocket = new QUdpSocket;
    if(!uDataSocket->bind(QHostAddress(cIP), dataPort,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint))
            qDebug()<<"bind failed--";
    if(!uDataSocket->joinMulticastGroup(QHostAddress("224.2.2.3")))
        qDebug()<<"joinMuticastGroup failed--";
    uDataSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,1024*1024*64);
    uDataSocket->setMulticastInterface(intf);

    connect(uDataSocket,SIGNAL(readyRead()),this,SLOT(recvProjection()),Qt::DirectConnection);
    exec();
}

void UDataSocket::leaveMGroup()
{
    uDataSocket->leaveMulticastGroup(QHostAddress("224.2.2.3"), intf);
}

void UDataSocket::recvProjection()
{
    char *recvBuf = new char[1052];
    memset(recvBuf, 0, 1052);
    while(uDataSocket->hasPendingDatagrams())
    {
        memset(recvBuf,0,1052);
        int fileSize = uDataSocket->pendingDatagramSize();
//        qDebug() << "fileSize:" << fileSize;

        uDataSocket->readDatagram(recvBuf, fileSize);
        ImageFrameHead *frame = (ImageFrameHead *)recvBuf;

        qDebug() << "pCount" << pCount << frame->uDataFrameCurr;

        if(pCount != (int)frame->uDataFrameCurr)  //丢包
        {
            memset(buf, 0, 1024*1024);
            pCount = 1;
            continue;
        }
        if(frame->funCode == 0)
        {
            pCount++;
            memcpy(buf+frame->uDataInFrameOffset, (recvBuf+sizeof(ImageFrameHead)), frame->uTransFrameCurrSize);
            qDebug() <<"curr"<<frame->uDataFrameCurr << frame->uDataFrametotal;
            if(frame->uDataFrameCurr == frame->uDataFrametotal)
            {
                pCount = 1;
                emit recvProjectionOk(buf, frame->uDataFrameSize);
            }
        }
    }
    delete recvBuf;
}

qint64 UDataSocket::write(char *data, qint64 size, QHostAddress address)
{
    return uDataSocket->writeDatagram(data,size,address,dataPort);
}
