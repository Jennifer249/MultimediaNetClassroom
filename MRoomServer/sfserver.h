#ifndef FILESERVER_H
#define FILESERVER_H

#include <QDialog>
#include <QTcpServer>
#include <QFile>
#include <QList>
#include "sfsocketthread.h"

namespace Ui {
class SFServer;
}

class SFServer : public QDialog
{
    Q_OBJECT

public:
    explicit SFServer(quint16 sFilePort = 16663,QWidget *parent = 0);
    ~SFServer();

private:
    Ui::SFServer *ui;
    QTcpServer *tcpServer;
    QString fileName;
    QString theFileName;

    quint16 sFilePort;

protected:
    void initServerWin();
    void closeEvent(QCloseEvent *e);
private slots:
    void on_sendBtn_clicked();
    void on_openBtn_clicked();
    void on_closeBtn_clicked();
    void updateProgressBar(qint64,qint64,QString);
    void newUsers();
    void fileWarning(QString);
signals:
    void sendFile(QString);
    void closeThread();
};

#endif // FILESERVER_H
