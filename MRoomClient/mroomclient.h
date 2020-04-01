#ifndef MROOMCLIENT_H
#define MROOMCLIENT_H

#include <QDialog>
#include <QUdpSocket>
#include "header.h"
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>
#include <QTcpSocket>
#include <tdatasocket.h>
#include <QHostAddress>
#include "sfile.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>
#include "udatasocket.h"
#include <QTimer>
#include "HBScreenShot.h"
#include <QFile>
#include <QPoint>
namespace Ui {
class MRoomClient;
}

enum MsgType{UserEnter, Msg, UserLeft, PrivateChat, UserList};

class MRoomClient : public QDialog
{
    Q_OBJECT

public:
    explicit MRoomClient(QWidget *parent = 0);
    ~MRoomClient();
    bool setUserName(QString);
    void setLoginNOState();
    void init();                             //初始化界面

private:
    void connControlPort();                  //创建主控连接
    void sendConMsg(QString, QHostAddress);  //主控端口发送消息
    qint64 writeChatMsg(MsgType, QString);    //聊天消息发送
    void connDataPort();                     //连接数据端口
    void userEnter(QString);
    void userLeft(QString, QString);
    void updateMsgTextBrowser(QString, QString, QString);
    void sendError(QString);
    void sendMsg(MsgType,QString);
    void updateUserList(QString);
    void showPrivateChat(QString,QString);
    void changeUserCount();
    void closeEvent(QCloseEvent *e);
    void createProjectionWidget();           //创建教师投影屏幕
    void setIP();
    //远程控制
    void mousepress(bool LR,int x,int y);
    void mousemove(int x,int y);
    void mouserelease(bool LR,int x,int y);
    void mousedoubleclick(int x, int y);
    void keybord(bool PR,int key,QString text);
    int translateKeyCode(int key);

private slots:
    void recvConMsg();                       //主控端口接收消息
    void on_sendBtn_clicked();
    void sendQuit();
    void dealChatMsg(int, QString, QString, QString);
    void connectOK();
    void tools(int);
    void privateChatChange(int);
    void showWindows();                     //显示界面
    void recvFileOK();
    void showProjection(char*,unsigned int);
    void checkHeart();
    void monitor();
    void recvPacket(int);
    void remoteControl();

private:
    Ui::MRoomClient *ui;
    quint16 controlPort;
    quint16 dataPort;
    QUdpSocket *controlSocket;
    TDataSocket *tDataSocket;
    PUserInfo userInfo;
    bool conConnectState;
    QTimer* heartTimer;
    QNetworkInterface intf;
    QString cIP;
    //托盘
    QSystemTrayIcon *systemTray;
    QAction *chatAct;
    QAction *quitAct;
    QMenu *pContextMenu;
    //聊天
    bool groupChatState;
    int userCount;
    bool loginOKState;  //判断登录状态，防止多次发送用户列表
    bool loginNOState;
    SFile* sFile;
    //投影
    UDataSocket* uDataSocket;
    quint16 projectionPort;
    QWidget *projectionWidget;
    QVBoxLayout *vBoxLayout;
    QLabel *projectionLabel;
    //监视
    HBScreenShot *hBScr;
    QTimer *mTimer;
    //远程控制
    QTimer *rTimer;
    QPoint pos;
    QCursor mouse;
    int winW;
    int winH;

signals:
    void loginOK();
    void loginNO();
};

#endif // MROOMCLIENT_H
