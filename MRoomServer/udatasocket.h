#ifndef UDATASOCKET_H
#define UDATASOCKET_H

#include <QThread>
#include <QUdpSocket>
#include <QNetworkInterface>
#include "monitorthread.h"
class UDataSocket : public QThread
{
    Q_OBJECT
public:
    UDataSocket(quint16 port, QNetworkInterface intf, QString sIP, QObject* parent = 0);
    qint64 write(char*, qint64, QHostAddress);
protected:
    virtual void run();

private:
    QUdpSocket* uDataSocket;
    quint16 dataPort;
    char *buf;
    int pCount;
    QNetworkInterface intf;
    QList<MonitorThread*> mUserList;
    QObject *parent;
    QString sIP;

private slots:
    void recvPicture();

signals:
    void recvMonitorOk(QString, char*, int);
    void recvRemoteControlOk(char*, int);
};

#endif // UDATASOCKET_H
