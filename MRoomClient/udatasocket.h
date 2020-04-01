#ifndef UDATASOCKET_H
#define UDATASOCKET_H

#include <Qthread>
#include <QUdpSocket>
#include <QNetworkInterface>
//#include "projectionthread.h"
class UDataSocket : public QThread
{
    Q_OBJECT
public:
    UDataSocket(quint16 port, QNetworkInterface intf, QString cIP, QObject* parent = 0);
    ~UDataSocket();
    qint64 write(char*, qint64, QHostAddress);
    void leaveMGroup();

protected:
    virtual void run();

public slots:
    void recvProjection();

signals:
    void recvProjectionOk(char*,unsigned int);
    void recvPacket(int);
private:
    QUdpSocket* uDataSocket;
    quint16 dataPort;
    char *buf;
    int pCount;
    QNetworkInterface intf;
//    ProjectionThread *projectionThread;
    QString cIP;
    int lastCount;
};

#endif // UDATASOCKET_H
