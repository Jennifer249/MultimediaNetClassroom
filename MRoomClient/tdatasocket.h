#ifndef TDATASOCKET_H
#define TDATASOCKET_H

#include <QTcpSocket>
#include <QDataStream>
#include <QHostAddress>
#include <QFile>
#include <QTime>
class TDataSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit TDataSocket(QHostAddress serIP, quint16 dataPort, QObject *parent = nullptr);
    void setUserName(QString);
    QString getUserName();
    void writeFile(QString);
private:
    void updatePending(int);
    void sendProgressMsg();
private slots:
    void continueSend(qint64);
private:
    //tcp沾包处理
    QByteArray tRecvData;
    qint64 tDataSize;
    qint64 tCheckSize;
    QByteArray tPendingBuffer;
    QString userName;
    int socketDescriptor;
    //写文件
    QFile* localFile;
    qint64 totalBytes;      //总共
    qint64 bytesWritten;    //已经写的
    qint64 bytesTobeWrite;  //剩下的
    qint64 payloadSize;
    QTime time;
    QByteArray fileBlock;
    bool fileState;
    //接收文件
    qint64 recvTotalBytes;
    qint64 bytesReceived;
    qint64 fileNameSize;
    QString fileName;
    QFile *locFile = Q_NULLPTR;
    quint16 blockSize;
    int pendingDataSize;
signals:
    void dealChatMsg(int, QString, QString, QString);
    void recvFileOK();
    void fileWarning(QString);
    void updateProgressBar(qint64,qint64,QString);

public slots:
    void recvTDataMsg();
};

#endif // TDATASOCKET_H
