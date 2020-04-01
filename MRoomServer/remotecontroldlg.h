#ifndef REMOTECONTROLDLG_H
#define REMOTECONTROLDLG_H

#include <QDialog>
#include <QMouseEvent>
#include <QUdpSocket>
#include <QKeyEvent>
namespace Ui {
class RemoteControlDlg;
}

class RemoteControlDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteControlDlg(QString userName, QString userIP, quint16 controlPort, QWidget *parent = 0);
    ~RemoteControlDlg();
    void setSocket(QUdpSocket *);
    void showRemoteControl(char *buf, int length);

private:
    void sendRemoteMsg(QString, QString, QString);

protected:
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    Ui::RemoteControlDlg *ui;
    QUdpSocket *udpSocket;
    QString userName;
    QString userIP;
    quint16 controlPort;
    QPixmap pixmap;
protected:
    void paintEvent(QPaintEvent *event);
};

#endif // REMOTECONTROLDLG_H
