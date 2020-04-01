#ifndef PCLIENTTHREAD_H
#define PCLIENTTHREAD_H

#include <Qthread>
#include <QUdpSocket>
#include "header.h"
class MServerThread : public QThread
{
    Q_OBJECT
public:
    MServerThread(char userName[], QObject *parent = 0);
    ~MServerThread();
    char* getUserName();
    void recvMonitor(char* imageBuf);
    int cou;

protected:
    virtual void run();

private:
    QUdpSocket *udpSocket;
    quint16 monitorPort;
    char *buf;
    char *userName;

signals:
    void recvMonitorOk(char*, char*, int);

};

#endif // PCLIENTTHREAD_H
