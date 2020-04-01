#include "tdatasocket.h"
TDataSocket::TDataSocket(QHostAddress serIP, quint16 dataPort, QObject *parent) : QTcpSocket(parent)
{
    this->connectToHost(serIP, dataPort);
    connect(this, SIGNAL(readyRead()),this,SLOT(recvTDataMsg()));
    connect(this,SIGNAL(bytesWritten(qint64)),this,SLOT(continueSend(qint64)));

    fileState = false;
    payloadSize = 64*1024;
    totalBytes = 0;
    bytesWritten = 0;
    bytesTobeWrite = 0;

    recvTotalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;
    blockSize = 0;
}

void TDataSocket::recvTDataMsg()
{
    if(this->bytesAvailable() <= 0)
        return;

    //临时读取缓冲区的所有数据
    QByteArray tempBuffer;
    tempBuffer = this->readAll();
    //上次缓冲加上这次的数据
    tPendingBuffer.append(tempBuffer);
    ushort CMD;

    pendingDataSize = tPendingBuffer.size();
    while(pendingDataSize)
    {
        QDataStream in(tPendingBuffer);
        in.setByteOrder(QDataStream::BigEndian);

        //不够包头的数据不处理
        if(pendingDataSize < (int)sizeof(ushort))
            break;

        in >> CMD;

        qDebug() <<tempBuffer.size()<< CMD << pendingDataSize;

        //数据足够多则进行处理
        if(CMD == (ushort)0)
        {
            quint64 len;
            in >> len;
            if(pendingDataSize < (int)len)
                break;
            int type;
            QString msg = NULL;
            QString time;
            QString userName;
            in >> type >> userName >> msg >> time;
//            qDebug() << type << userName << msg << time;
            emit dealChatMsg(type, userName, msg, time);
            //总的数据，前面部分已处理，将后面部分缓存
            tempBuffer = tPendingBuffer.right(pendingDataSize - len);
            //更新长度
            pendingDataSize = tempBuffer.size();
            //更新待处理的数据
            tPendingBuffer = tempBuffer;
        }
        else if(CMD == (ushort)1)
        {
            if (bytesReceived <= sizeof(qint64)*2+(int)sizeof(ushort)) //先获得前两个数据，文件的大小、文件名的大小
            {
                if ((pendingDataSize >= (int)sizeof(qint64)*2+(int)sizeof(ushort)) && (fileNameSize == 0))
                {
                    in >> recvTotalBytes >> fileNameSize;   //获得文件的大小、文件名的大小
                    bytesReceived += sizeof(qint64)*2+(int)sizeof(ushort);
                    pendingDataSize -= (int)sizeof(qint64)*2+(int)sizeof(ushort);
                }
                if((pendingDataSize >= fileNameSize) && (fileNameSize != 0)){   //获取文件名
                    in >> fileName;
                    bytesReceived += fileNameSize;
                    pendingDataSize -= fileNameSize;
                }
                else
                {
                    updatePending(bytesReceived);
                    //数据不足等下次
                    return;
                }
                updatePending(bytesReceived);
            }
            if(!fileName.isEmpty() && locFile == Q_NULLPTR)
            {
                locFile = new QFile(fileName);
                if(!locFile->open(QFile::WriteOnly))
                {
                    qDebug()<< "File Open Failed";
                    delete locFile;
                    locFile = Q_NULLPTR;
                    return;
                }
            }

            if(locFile == Q_NULLPTR)
                return;

            int writeMin = qMin(pendingDataSize-(int)sizeof(ushort), (int)(recvTotalBytes-bytesReceived));
            if (bytesReceived < recvTotalBytes) {
                bytesReceived += writeMin;
                qDebug() << "writeMin" << writeMin;
                locFile->write(tPendingBuffer.right(pendingDataSize-(int)sizeof(ushort)), writeMin);
                updatePending(writeMin+(int)sizeof(ushort));
            }

            if(bytesReceived == recvTotalBytes)
            {
                locFile->close();
                locFile = Q_NULLPTR;
                fileName = "";
                recvTotalBytes = 0;
                bytesReceived = 0;
                fileNameSize = 0;
                blockSize = 0;
                tempBuffer = tPendingBuffer.right(pendingDataSize-(int)sizeof(ushort));
                //更新长度
                pendingDataSize = tempBuffer.size();
                //更新待处理的数据
                tPendingBuffer = tempBuffer;
                qDebug() << pendingDataSize;
                emit recvFileOK();
            }
            return;
        }
    }
}

void TDataSocket::setUserName(QString userName)
{
    this->userName = userName;
}

QString TDataSocket::getUserName()
{
   return userName;
}

//更新缓冲区数据
void TDataSocket::updatePending(int len)
{
    QByteArray temp;
    QDataStream out(&temp,QIODevice::WriteOnly);
    out << (ushort)1;
    //总的数据，前面部分已处理，将后面部分缓存
    temp.append(tPendingBuffer.right(tPendingBuffer.size()-len));
    //更新长度
    pendingDataSize = temp.size();
    //更新待处理的数据
    tPendingBuffer = temp;
}

//发送文件
void TDataSocket::writeFile(QString filePath)
{
    fileState = true;
    localFile = new QFile(filePath);
    if(!localFile->open(QFile::ReadOnly))
    {
        emit fileWarning(localFile->errorString());
        return;
    }
    totalBytes = localFile->size(); //文件大小

    qDebug() << "totalBytes" <<totalBytes;

    QDataStream out(&fileBlock,QIODevice::WriteOnly);
    time.start();   //计时
    QString currentFile = filePath.right(filePath.size()-filePath.lastIndexOf('/')-1); //匹配从右边开始的子项
    out << ushort(0) << qint64(0) << qint64(0) << currentFile;  //14+14+16 首部
    totalBytes += fileBlock.size();  //原文件大小+首部大小
    out.device()->seek(0);
    ushort CMD = (ushort)1;  //文件
    out << CMD << totalBytes << qint64(fileBlock.size()-sizeof(qint64)*2-sizeof(ushort)); //写入总文件大小，文件名大小
    qDebug() << CMD << totalBytes << qint64(fileBlock.size()-sizeof(qint64)*2-sizeof(ushort));
    bytesTobeWrite = totalBytes - write(fileBlock);   //发送首部后，计算剩余大小
    fileBlock.resize(0);
}

void TDataSocket::continueSend(qint64 numBytes)
{
    if(fileState == false)
        return;
    bytesWritten += (int)numBytes;
    sendProgressMsg();
    if(bytesTobeWrite > 0)
    {
        fileBlock = localFile->read(qMin(bytesTobeWrite,payloadSize));   //读取该大小的数据
        bytesTobeWrite -= (int)write(fileBlock);  //写入
        qDebug() << "bytesTobeWrite" <<bytesTobeWrite << qMin(bytesTobeWrite,payloadSize);
    }
    else
    {
        if(localFile->isOpen())
            localFile->close();
        fileBlock.resize(0);
        fileState = false;
        totalBytes = 0;
        bytesWritten = 0;
        bytesTobeWrite = 0;
    }

    if(bytesWritten == totalBytes)
    {
        if(localFile->isOpen())
            localFile->close();
        fileBlock.resize(0);
        fileState = false;
        totalBytes = 0;
        bytesWritten = 0;
        bytesTobeWrite = 0;
    }
}

void TDataSocket::sendProgressMsg()
{
    float useTime = time.elapsed();
    double speed = bytesWritten / useTime;
    QString msg = tr("已发送%1MB(%2MB/s \n共%3MB 已用时：%4秒\n估计剩余时间：%5s)")
            .arg((double)bytesWritten/(1024*1024),0,'f',2)
            .arg(speed*1000/(1024*1024),0,'f',2)
            .arg((double)totalBytes/(1024*1024),0,'f',2)
            .arg(useTime/1000,0,'f',0)
            .arg((double)totalBytes/speed/1000 - useTime/1000,0,'f',0);
    emit updateProgressBar(totalBytes,bytesWritten,msg);
}

