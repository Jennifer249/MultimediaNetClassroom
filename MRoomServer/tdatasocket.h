#ifndef TDATASOCKET_H
#define TDATASOCKET_H

#include <QTcpSocket>
#include <QDataStream>
#include <QObject>
#include <QThread>
#include <QFile>
#include <QTime>
class TDataSocket : public QThread
{
    Q_OBJECT
public:
    explicit TDataSocket(QTcpSocket *tSocket, QObject *parent = 0);
    void setUserName(QString);                         //设置用户名
    QString getUserName();                             //获取用户名
    void writeChatMsg(int, QString, QString, QString);  //发送聊天消息
    void writeFile(QString);
private:
    void updatePending(int);
signals:
    void dealChatMsg(TDataSocket*, int, QString);      //发送待处理聊天消息信号
    void setUserIP(TDataSocket*, QString);
    void removeUser(QString, QString);                          //发送移除用户消息
    void fileWarning(QString);
    void updateProgressBar(QString, qint64,qint64,float);
    void recvFileOK();
private slots:
    void recvTDataMsg();                               //接收消息
    void ConnectErr();                                 //连接错误
    void dealClose();                                      //关闭tcpSocket，被调用
    void continueSend(qint64);
protected:
    virtual void run();

private:
    //tcp沾包处理
    QByteArray tRecvData;
    qint64 tDataSize;
    qint64 tCheckSize;
    QByteArray tPendingBuffer;
    QTcpSocket* tcpSocket;
    QString userName;
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
};

#endif // TDATASOCKET_H
