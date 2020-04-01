#include "udatasocket.h"

UDataSocket::UDataSocket(quint16 port, QNetworkInterface intf, QString sIP, QObject *parent)
    :QThread(parent)
{
    buf = new char[1024*1024];
    memset(buf, 0, 1024*1024);
    dataPort = port;
    pCount = 1;
    this->intf = intf;
    this->sIP = sIP;
}

void UDataSocket::run()
{
    uDataSocket = new QUdpSocket;
    if(!uDataSocket->bind(QHostAddress(sIP), dataPort,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint))
            qDebug()<<"bind failed--";
    uDataSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,1024*1024*8);
    uDataSocket->setMulticastInterface(intf);
    connect(uDataSocket, SIGNAL(readyRead()), this, SLOT(recvPicture()),Qt::DirectConnection);
    exec();
}

//分发收到的UDP帧，分别处理
void UDataSocket::recvPicture()
{
    char *recvBuf = new char[1116];
    memset(recvBuf,0,1116);
    while(uDataSocket->hasPendingDatagrams())
    {
        bool flag = false;
        memset(recvBuf,0,1116);
        int pDataSize = uDataSocket->pendingDatagramSize();
        //qDebug() << "dataSize:" << pDataSize;

        uDataSocket->readDatagram(recvBuf, pDataSize);
        ImageUserInfo *imageUserInfo = (ImageUserInfo *)recvBuf;

        qDebug() << "imageUserInfo->userName" << imageUserInfo->userName;
        if(imageUserInfo->CMD == 0)
        {
            char *imageBuf = new char[imageUserInfo->dataSize];
            memcpy(imageBuf, (recvBuf+sizeof(ImageUserInfo)), imageUserInfo->dataSize);
            for(int i=0; i<mUserList.count(); i++)
            {
               MonitorThread *item = mUserList.at(i);
               qDebug() <<"imageUserInfo->userName" <<imageUserInfo->userName << mUserList.count()<<"item->getUserName()"<<item->getUserName();
               if(QString::compare(QString(imageUserInfo->userName), item->getUserName())==0)
               {
                   flag = true;
                   item->recvMonitor(imageBuf);
               }
            }
            if(flag == false)
            {
                QThread thread;
                MonitorThread *newM = new MonitorThread(QString(imageUserInfo->userName));
                newM->moveToThread(&thread);
                connect(newM,SIGNAL(recvMonitorOk(QString, char*, int)),this,SIGNAL(recvMonitorOk(QString, char*, int)));
                mUserList.append(newM);
                newM->recvMonitor(imageBuf);
                qDebug() <<"imageUserInfo->userName" <<imageUserInfo->userName<<mUserList.count();
            }
        }
        else if(imageUserInfo->CMD == 1)
        {
            ImageFrameHead *frame = (ImageFrameHead *)(imageUserInfo+1);
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
                memcpy(buf+frame->uDataInFrameOffset, (recvBuf+sizeof(ImageUserInfo)+sizeof(ImageFrameHead)), frame->uTransFrameCurrSize);
                if(frame->uDataFrameCurr == frame->uDataFrametotal)
                {
                    pCount = 1;
                    qDebug() << "==ok" <<imageUserInfo->userName;
                    emit recvRemoteControlOk(buf, frame->uDataFrameSize);
        //            memset(buf, 0, 1024);
                }
            }
        }

    }
    delete recvBuf;
}

qint64 UDataSocket::write(char *data, qint64 size, QHostAddress address)
{
    return uDataSocket->writeDatagram(data,size,address,dataPort);
}
