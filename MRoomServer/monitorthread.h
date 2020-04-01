#ifndef MONITORTHREAD_H
#define MONITORTHREAD_H

#include <Qthread>
#include <QUdpSocket>
#include "header.h"
class MonitorThread : public QObject
{
    Q_OBJECT

public:
    MonitorThread(QString userName, QObject *parent = 0);
    ~MonitorThread();
    QString getUserName();
    void recvMonitor(char* imageBuf);


private:
    quint16 monitorPort;
    char *buf;
    QString userName;
    int pCount;

signals:
    void recvMonitorOk(QString, char*, int);
};

#endif // MONITORTHREAD_H
