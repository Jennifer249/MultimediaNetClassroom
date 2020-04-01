#ifndef MROOMSERVER_H
#define MROOMSERVER_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpServer>
#include <QGridLayout>
#include <QTcpSocket>
#include "tdatasocket.h"
#include <QListWidgetItem>
#include "sfile.h"
#include "HBScreenShot.h"
#include <QNetworkInterface>
#include <QTimer>
#include "monitorthread.h"
#include "udatasocket.h"
#include <QMouseEvent>
#include "remotecontroldlg.h"
namespace Ui {
class MRoomServer;
}
enum MsgType{UserEnter, Msg, UserLeft, PrivateChat, UserList};

class MRoomServer : public QMainWindow
{
    Q_OBJECT

public:
    explicit MRoomServer(QWidget *parent = 0);
    ~MRoomServer();

private:
    void init();                                                 //初始化界面
    void startServer();                                          //开启所有端口服务
    void sendConMsg(QString, QString, QHostAddress);             //发送主控端口消息
    void judgeUsers(QString);                                    //判断用户是否重复
    QString getIP();                                             //获取IP地址
    void sendConBroMsg(QString, QString, quint16);               //广播所有网卡消息
    //处理聊天服务器
    void userEnter(QString);                                     //处理新用户加入
    void userLeft(QString, QString);                             //处理用户离开
    void showMsg(QString, QString, QString);                     //处理用户发送的消息
    void showPrivateMsg(QString, QString, QString);              //处理用户发送的消息
    void judgeChatUsers(QString, QHostAddress);                  //判断新增的聊天用户是否重复
    PUserInfo searchUsers(QString);                              //寻找匹配用户
    void changeUserNum();                                        //改变用户列表的在线用户个数
    void broadcastMsg(TDataSocket*, MsgType, QString, QString);  //广播消息
    bool openProjection();
    bool checkProjection();
    //监视
    void createMonitorLabel(QString monitorName, int column);  //创建监视界面
    void setIP();
    //远程控制
    void createRemoteControlWidget();

private slots:
    void recvConMsg();                                           //接收主控端口消息
    void dealChatMsg(TDataSocket*, int, QString);                //处理聊天消息
    void setUserIP(TDataSocket*, QString);
    void on_sendBtn_clicked();                                   //点击发送聊天消息按钮
    void showUserList();                                         //显示用户列表窗口
    void showChat();                                             //显示聊天窗口
    void newConnection();                                        //新用户连接
    void groupTalk();                                            //标记群聊
    void privateChatChange(QListWidgetItem* item);               //标记私聊
    void removeUser(QString, QString);                           //移除用户
    void sendFile();                                             //发送文件
    void projection();                                           //投影
    void sendHeart();
    void monitor();                                              //监视
    void showMonitor(QString, char*, int);
    void recvFileOK();
    //远程控制
    void remoteControl();
    void on_userListWidget_customContextMenuRequested(const QPoint &pos);
    void showRemoteControl(char*, int);

protected:
    void paintEvent(QPaintEvent *event);

private:
    Ui::MRoomServer *ui;
    //数据
    quint16 controlPort;
    quint16 dataPort;
    QUdpSocket *controlSocket;
    UDataSocket *uDataSocket;
    QTcpServer *tDataServer;
    QString sIP;
    QList<PUserInfo> userInfoList;
    QTimer* heartTimer;
    QNetworkInterface intf;
    //界面
    QAction *sendFileAction;
    QAction *projectAction;
    QAction *monitorAction;
    QAction *showUserListAction;
    QAction *showChatAction;
    QMenu *toolMenu;
    QMenu *viewMenu;
    QGridLayout *scrollGridLayout;
    //聊天
    bool groupChatState;
    TDataSocket* privateTalkerSocket;
    //文件
    SFile* sFile;
    //投影
    bool projectionState;
    HBScreenShot *hBScr;
    int packetSum;
    int pIndex;
    QList<QString> pCheckNames;
    char *pixChar;
    ImageFrameHead frame;
    QPixmap pix;
    //监视
    bool monitorState;
    QList<MUserWidget *> mUserWidgetList;
    //远程控制
    QWidget *remoteControlWidget;
    QVBoxLayout *rvBoxLayout;
    QLabel *remoteControlLabel;
    QTimer *pointTimer;
    QString remoteControlUser;
    bool remoteControlState;
    RemoteControlDlg* remoteControlDlg;

};

#endif // MROOMSERVER_H
