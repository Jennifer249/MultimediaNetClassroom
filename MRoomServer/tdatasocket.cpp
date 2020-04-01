#include "tdatasocket.h"
#include <QDateTime>

TDataSocket::TDataSocket(QTcpSocket* tSocket, QObject *parent)
    :QThread(parent)
{
    tcpSocket = tSocket;
    fileState = false;
    QObject::connect(tcpSocket, SIGNAL(readyRead()),this,SLOT(recvTDataMsg()));
    QObject::connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(ConnectErr()));
    connect(tcpSocket,SIGNAL(bytesWritten(qint64)),this,SLOT(continueSend(qint64)));

    //发送文件
    payloadSize = 64*1024;
    totalBytes = 0;
    bytesWritten = 0;
    bytesTobeWrite = 0;

    recvTotalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;
    blockSize = 0;
}

void TDataSocket::run()
{
    exec();
}

//处理接收消息
void TDataSocket::recvTDataMsg()
{
    if(tcpSocket->bytesAvailable() <= 0)
        return;

    //临时读取缓冲区的所有数据
    QByteArray tempBuffer;
    tempBuffer = tcpSocket->readAll();
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
            in >> type >> msg;
            if(type == 0)
            {
                QString IP = NULL;
                in >> IP;
                qDebug() << "userIP" << IP;
                emit setUserIP(this,IP);
            }
            emit dealChatMsg(this, type, msg);
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
                locFile = new QFile(userName+"_"+fileName);
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

//发送聊天消息
void TDataSocket::writeChatMsg(int type, QString userName, QString msg, QString time)
{
    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << ushort(0) << quint64(0) << type << userName << msg << time;
    out.device()->seek(0);
    ushort CMD = 0;  //聊天
    quint64 len = (quint64)block.size();
    out << CMD << len;
    qDebug() << "send:" << CMD << len;
    tcpSocket->write(block);
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
    bytesTobeWrite = totalBytes - tcpSocket->write(fileBlock);   //发送首部后，计算剩余大小
    fileBlock.resize(0);
}

void TDataSocket::continueSend(qint64 numBytes)
{
    if(fileState == false)
        return;
    bytesWritten += (int)numBytes;

    //发送进度条
    float useTime = time.elapsed();
    emit updateProgressBar(userName,totalBytes,bytesWritten,useTime);

    if(bytesTobeWrite > 0)
    {
        fileBlock = localFile->read(qMin(bytesTobeWrite,payloadSize));   //读取该大小的数据
        bytesTobeWrite -= (int)tcpSocket->write(fileBlock);  //写入
        qDebug() << "bytesTobeWrite" << bytesTobeWrite << qMin(bytesTobeWrite,payloadSize);
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

//设置tcpSocket的用户姓名
void TDataSocket::setUserName(QString userName)
{
    this->userName = userName;
}

//获取该tcpSocket的用户姓名
QString TDataSocket::getUserName()
{
   return userName;
}

//关闭tcpSocket
void TDataSocket::dealClose()
{
    qDebug() << "dealClose";
    tcpSocket = NULL;
}

//tcpSocket连接错误消息
void TDataSocket::ConnectErr()
{
    tcpSocket = NULL;
    qDebug() << "服务器的 tDataSocket 连接错误";
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
    emit removeUser(userName, time);
}
