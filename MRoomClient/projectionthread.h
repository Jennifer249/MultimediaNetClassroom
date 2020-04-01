#ifndef PROJECTIONTHREAD_H
#define PROJECTIONTHREAD_H
#include <QUdpSocket>
class ProjectionThread : public QObject
{
    Q_OBJECT
public:
    explicit ProjectionThread(QObject *parent = nullptr);
    void recvProjection(char* imageBuf);
    ~ProjectionThread();
signals:
    void recvProjectionOk(char*, int);
    void lostPacket(int);
private:
    quint16 monitorPort;
    char *buf;
    QString userName;
    int pCount;
};

#endif // PROJECTIONTHREAD_H
