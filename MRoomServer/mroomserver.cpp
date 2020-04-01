#include "mroomserver.h"
#include "ui_mroomserver.h"
#include <QHostInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QPainter>
#include <QBuffer>
#include <QSettings>
MRoomServer::MRoomServer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MRoomServer)
{
    ui->setupUi(this);

    //初始化参数
    controlPort = 16660;
    dataPort = 16661;
    groupChatState = true;
    projectionState = false;
    monitorState = false;
    remoteControlState = true;
    heartTimer = new QTimer();
    pointTimer =  new QTimer();
    packetSum = 20;
    pIndex = 0;
    //初始化界面
    init();

    //开启所有端口服务
    startServer();
}


MRoomServer::~MRoomServer()
{
    delete ui;
}

/* [0]初始化*/

//界面初始化
void MRoomServer::init()
{
    //创建工具条
    sendFileAction = new QAction("传输文件",this);
    projectAction = new QAction("开启投影",this);
    monitorAction = new QAction("开启监视",this);
    showUserListAction = new QAction("显示在线用户",this);
    showChatAction = new QAction("显示聊天窗口",this);
    connect(showUserListAction,SIGNAL(triggered()),this,SLOT(showUserList()));
    connect(showChatAction,SIGNAL(triggered()),this,SLOT(showChat()));
    connect(sendFileAction,SIGNAL(triggered()),this,SLOT(sendFile()));
    connect(projectAction,SIGNAL(triggered()),this,SLOT(projection()));
    connect(monitorAction,SIGNAL(triggered()),this,SLOT(monitor()));

    //创建菜单
    toolMenu = menuBar()->addMenu(tr("工具"));
    toolMenu->addAction(sendFileAction);
    toolMenu->addAction(projectAction);
    toolMenu->addAction(monitorAction);
    viewMenu = menuBar()->addMenu(tr("窗口"));
    viewMenu->addAction(showUserListAction);
    viewMenu->addAction(showChatAction);

    //初始化中心窗口的滚动区
    scrollGridLayout = new QGridLayout();
    ui->monitorScrollArea->setLayout(scrollGridLayout);
    ui->scrollArea->setWidgetResizable(true);

    //颜色
//    QPalette palette;
//    palette.setBrush(QPalette::Window, QBrush(Qt::white));
//    ui->centralWidget->setPalette(palette);

    //聊天方式：私聊
    connect(ui->userListWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(privateChatChange(QListWidgetItem*)));

    //聊天方式：群聊
    connect(ui->groupRadioBtn,SIGNAL(clicked()),this,SLOT(groupTalk()));
}

//开启端口
void MRoomServer::startServer()
{
    //开启主控端口

    setIP();
    controlSocket = new QUdpSocket(this);
    controlSocket->bind(QHostAddress(sIP), controlPort,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);

    connect(controlSocket, SIGNAL(readyRead()), this, SLOT(recvConMsg()));
    controlSocket->setMulticastInterface(intf);
    controlSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,1024*1024*8);

    if (!controlSocket->joinMulticastGroup(QHostAddress("224.2.2.2")))
    {
        QMessageBox::critical(this, "error", "join multicast group failed");
        exit(0);
    }

    //开启tcp数据端口的服务器
    tDataServer = new QTcpServer(this);
    if(!tDataServer->listen(QHostAddress::AnyIPv4, dataPort))
    {
        qDebug()<< "wrong!";
    }
    connect(tDataServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    userEnter("Teacher");  //显示老师登录

    //开启udp数据端口的服务器
    uDataSocket = new UDataSocket(dataPort, intf, sIP);
    uDataSocket->start();
    connect(uDataSocket,SIGNAL(recvMonitorOk(QString, char*, int)),this,SLOT(showMonitor(QString, char*, int)));
    connect(uDataSocket,SIGNAL(recvRemoteControlOk(char*, int)),this,SLOT(showRemoteControl(char*, int)));
    heartTimer->start(10000);
    connect(heartTimer, SIGNAL(timeout()), this, SLOT(sendHeart()));

    qDebug() << intf.name() <<sIP;
}

void MRoomServer::sendHeart()
{
    sendConMsg("Teacher", "CON_HEART", QHostAddress("224.2.2.2"));
}

//显示聊天窗口
void MRoomServer::showChat()
{
    ui->chatWidget->show();
}

//显示用户列表
void MRoomServer::showUserList()
{
    ui->userDockWidget->show();
}

/* ...初始化*/

/* [1]UDP主控端口处理*/
//接收消息
void MRoomServer::recvConMsg()
{
    while (controlSocket->hasPendingDatagrams())
    {
        QByteArray rdata;
        QHostAddress userAddr;
        rdata.resize(controlSocket->pendingDatagramSize());
        controlSocket->readDatagram(rdata.data(),rdata.size(),&userAddr);   //接收数据写入rdata
        QDataStream in(&rdata, QIODevice::ReadOnly);
        QString type;
        in >> type;
        if(type == "CON_ASKIP")
        {
            QString userName;
            in >> userName;
            qDebug() << "recv" << type << "userName" << userName <<"userAddr" <<userAddr;
            judgeUsers(userName);
        }
        else if(type == "PRO_OK")
        {
            QString uName;
            int pCount;
            in >> uName >> pCount;
            UserInfo *tempU = searchUsers(uName);

            qDebug() << type << uName << pCount <<pIndex << tempU->pCount;
            tempU->pCount = pCount-pIndex;
        }
    }
}

//发送消息
void MRoomServer::sendConMsg(QString userName, QString msg, QHostAddress userAddr)
{
    //使用：sendConMsg(userName, "CON_LOGINOK", userAddr);//同上，单播测试两台电脑，本机电脑不能收到,另外一台只能实例一个
    QByteArray sData;
    QDataStream out(&sData, QIODevice::WriteOnly);
    out << userName;
    out << msg;
    if(msg == "CON_LOGINOK")
    {
        out << sIP;
    }
//    qDebug() << "userName" << userName << "msg" << msg << "userAddr" << userAddr;
    controlSocket->writeDatagram(sData, sData.length(), userAddr, controlPort);
}

//向所有虚拟网口广播消息
void MRoomServer::sendConBroMsg(QString userName, QString msg, quint16 port)
{
    QByteArray sData;
    QDataStream out(&sData, QIODevice::WriteOnly);
    out << userName;
    out << msg;
    QHostInfo::fromName(QHostInfo::localHostName()).addresses().last().toString();
    QList<QNetworkInterface> networkinterfaces = QNetworkInterface::allInterfaces();

    foreach (QNetworkInterface interface, networkinterfaces)
    {
        foreach (QNetworkAddressEntry entry, interface.addressEntries())
        {
            QHostAddress broadcastAddress = entry.broadcast();
            if (broadcastAddress != QHostAddress::Null
                && entry.ip() != QHostAddress::LocalHost
                && entry.ip().protocol() == QAbstractSocket::IPv4Protocol
                )
            {
                qDebug() << "con"<<broadcastAddress.toString();
                    controlSocket->writeDatagram(sData, sData.length(), broadcastAddress, port);  // UDP 发送数据
            }
        }
    }
}

//判断新增用户是否重复
void MRoomServer::judgeUsers(QString userName)
{
    if(userName == "Teacher")
    {
        sendConMsg(userName, "CON_LOGINNO", QHostAddress("224.2.2.2"));
        return;
    }
    if(searchUsers(userName) != NULL)
    {
        sendConMsg(userName, "CON_LOGINNO", QHostAddress("224.2.2.2"));
        return;
    }
    sendConMsg(userName, "CON_LOGINOK", QHostAddress("224.2.2.2"));
}

/* ...UDP主控端口处理*/

/* [2]tcp数据端口*/

//新的TCP用户加入
void MRoomServer::newConnection()
{
    qDebug() << "newConnection() : tcp user come";

    //开启tcpSocket线程，并加入列表
    QTcpSocket* tempTcpSocket = tDataServer->nextPendingConnection();
    TDataSocket *tDataSocket = new TDataSocket(tempTcpSocket);
    tDataSocket->start();
    PUserInfo newUser = new UserInfo();
    newUser->tDataSocket = tDataSocket;
    userInfoList.append(newUser);
    connect(tDataSocket, SIGNAL(recvFileOK()), this, SLOT(recvFileOK()));
    connect(tDataSocket, SIGNAL(dealChatMsg(TDataSocket*, int, QString)), this, SLOT(dealChatMsg(TDataSocket*, int, QString)));
    connect(tDataSocket, SIGNAL(setUserIP(TDataSocket*, QString)), this, SLOT(setUserIP(TDataSocket*, QString)));
    connect(tDataSocket, SIGNAL(removeUser(QString, QString)), this, SLOT(removeUser(QString, QString)));
}

//处理聊天消息
void MRoomServer::dealChatMsg(TDataSocket* tDataSocket, int type, QString msg)
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
    switch ((MsgType)type) {
    case UserEnter:
        for(int i=0; i<userInfoList.count(); i++)
        {
            if(userInfoList.at(i)->tDataSocket == tDataSocket)
            {
                userInfoList.at(i)->userName = msg;
                break;
            }
        }
        tDataSocket->setUserName(msg);
        userEnter(msg);
        broadcastMsg(tDataSocket, (MsgType)type, msg, time);
        break;
    case Msg:
        showMsg(tDataSocket->getUserName(),msg,time);
        broadcastMsg(tDataSocket, (MsgType)type, msg, time);
        break;
    case PrivateChat:
        showPrivateMsg(tDataSocket->getUserName(),msg,time);
        break;
    default:
        break;
    }
}

void MRoomServer::setUserIP(TDataSocket* tDataSocket, QString ip)
{
    for(int i=0; i<userInfoList.count(); i++)
    {
        if(userInfoList.at(i)->tDataSocket == tDataSocket)
        {
            userInfoList.at(i)->userIP = ip;
            break;
        }
    }
}

//处理广播消息
void MRoomServer::broadcastMsg(TDataSocket* tDataSocket, MsgType type, QString msg, QString time)
{
    qDebug() << "broadcastMsg" << type << msg;
    //给新用户发送用户列表
    if(type == UserEnter)
    {
        QString userList;
        userList.append("Teacher");
        for(int i=0; i<userInfoList.count(); i++)
        {
            if(userInfoList.at(i)->userName == msg)
                continue;
            userList.append(";"+userInfoList.at(i)->userName);
        }
        tDataSocket->writeChatMsg(UserList, tDataSocket->getUserName(), userList, time);
    }
    if(tDataSocket == NULL)
    {
        for(int i=0; i<userInfoList.count(); i++)
        {
            userInfoList.at(i)->tDataSocket->writeChatMsg(type, "Teacher", msg, time);
        }
        return;
    }
    //给其他用户发送消息,群聊消息、登录
    for(int i=0; i<userInfoList.count(); i++)
    {
        userInfoList.at(i)->tDataSocket->writeChatMsg(type, tDataSocket->getUserName(), msg, time);
    }
}

/* ...tcp数据端口*/

/* [3]聊天*/

//界面更新:更新用户列表，移除用户
void MRoomServer::userLeft(QString username,QString time)
{
   //删除在线列表离开的用户
    for(int i=0; i< ui->userListWidget->count(); i++)
    {
        if(username == ui->userListWidget->item(i)->text())
        {
            ui->userListWidget->removeItemWidget(ui->userListWidget->takeItem(i));
            if(ui->status->text().contains(username,Qt::CaseSensitive))
            {
                ui->status->setText("");
                ui->groupRadioBtn->setChecked(true);
                groupChatState = true;
            }
        }
    }
    //聊天消息窗口中显示离开的用户
    ui->msgTextBrowser->setTextColor(Qt::gray);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman", 10));
    ui->msgTextBrowser->append(tr("%1 于 %2 离开！").arg(username).arg(time));
    changeUserNum();

    for(int i=0; i < mUserWidgetList.count(); i++)
    {
        MUserWidget *item = mUserWidgetList.at(i);
        if(item->nameLine->text() == username)
        {
            mUserWidgetList.removeAt(i);
            scrollGridLayout->removeWidget(item->mWidget);
            item->mWidget->deleteLater();
        }
    }
}

//从UseInfoList中，移除退出的用户
void MRoomServer::removeUser(QString username, QString time)
{
    //从UseInfoList中，关闭退出用户的tcpSocket
    for(int i=0; i<userInfoList.count(); i++)
    {
        if(username == userInfoList.at(i)->userName)
        {
            userInfoList.at(i)->tDataSocket->quit();
            userInfoList.at(i)->tDataSocket->wait();
            userInfoList.removeAt(i);
        }
    }
    userLeft(username,time);
    for(int i=0; i<userInfoList.count(); i++)
    {
        userInfoList.at(i)->tDataSocket->writeChatMsg(UserLeft, username, "", time);
    }
}

//界面更新:更新聊天窗口信息
void MRoomServer::showMsg(QString username, QString msg, QString time)
{
    ui->msgTextBrowser->setTextColor(Qt::blue);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",10));
    ui->msgTextBrowser->append("[ " +username+" ] "+ time);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",9));
    ui->msgTextBrowser->setTextColor(Qt::black);
    ui->msgTextBrowser->append(msg);
}

//界面更新:更新用户列表，新增用户
void MRoomServer::userEnter(QString username)
{
    bool isEmpty = ui->userListWidget->findItems(username, Qt::MatchExactly).isEmpty();
    if(isEmpty)
    {
        ui->userListWidget->addItem(username);
        ui->msgTextBrowser->setTextColor(Qt::gray);
        ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",10));
        ui->msgTextBrowser->append(tr("%1 在线！").arg(username));
    }
    changeUserNum();
    if(username != "Teacher")
        createMonitorLabel(username,3);
}

//界面更新:显示私聊信息
void MRoomServer::showPrivateMsg(QString username, QString msg, QString time)
{
    ui->msgTextBrowser->setTextColor(Qt::blue);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",10));
    ui->msgTextBrowser->append("私聊：[ " +username+" ] "+ time);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",9));
    ui->msgTextBrowser->setTextColor(Qt::black);
    ui->msgTextBrowser->append(msg);
}

//改变在线用户数
void MRoomServer::changeUserNum()
{
     ui->userDockWidget->setWindowTitle(tr("在线用户(%1)").arg(userInfoList.count()+1));
}

//标记私聊状态
void MRoomServer::privateChatChange(QListWidgetItem* item)
{
    QString privateTalker = item->text();
    if(privateTalker == "Teacher")  //老师选择自己，则恢复为群聊，其他为私聊对应学生
    {
        ui->status->setText("");
        ui->groupRadioBtn->setChecked(true);
        groupChatState = true;
    }
    else
    {
        ui->status->setText("私聊:" + privateTalker);
        ui->groupRadioBtn->setChecked(false);
        groupChatState = false;
        privateTalkerSocket = searchUsers(item->text())->tDataSocket;
    }
}

//标记群聊状态
void MRoomServer::groupTalk()
{
    groupChatState = true;
    ui->status->setText("");
}

//点击教师发送聊天消息按钮
void MRoomServer::on_sendBtn_clicked()
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
    if (ui->msgTextEdit->toPlainText() == "") {
        QMessageBox::warning(0,tr("警告"),tr("发送内容不能为空"),QMessageBox::Ok);
        return;
    }
    ui->msgTextBrowser->verticalScrollBar()->setValue(ui->msgTextBrowser->verticalScrollBar()->maximum());

    //选择聊天方式交给聊天线程处理：群聊or私聊
    if(groupChatState == true)
    {
        broadcastMsg(NULL, Msg, ui->msgTextEdit->toPlainText(), time);
        showMsg("Teacher", ui->msgTextEdit->toPlainText(), time);
    }
    else
    {
        privateTalkerSocket->writeChatMsg(PrivateChat,privateTalkerSocket->getUserName(),  ui->msgTextEdit->toPlainText(),time);
        showPrivateMsg(privateTalkerSocket->getUserName(),ui->msgTextEdit->toPlainText(),time);
    }
    ui->msgTextEdit->clear();
}

/* ...聊天*/

/* [4]文件*/

//触发发送文件按钮
void MRoomServer::sendFile()
{
    sFile = new SFile(userInfoList);
    sFile->setAttribute(Qt::WA_DeleteOnClose);
    sFile->show();
}

void MRoomServer::recvFileOK()
{
    QMessageBox::warning(0,tr("文件传输"),tr("文件已接收"),QMessageBox::Ok);
}

/*...文件*/

/* [5]投影*/

// 开启/关闭投影
void MRoomServer::projection()
{
    if(projectionState == false)
    {
        projectAction->setText("关闭投影");
        sendConMsg("Teacher", "CON_PROJECTION", QHostAddress("224.2.2.2"));
        projectionState = true;
        while(openProjection()){}
    }
    else
    {
        projectAction->setText("开启投影");
        sendConMsg("Teacher", "CON_STOP_PROJECTION", QHostAddress("224.2.2.2"));
        projectionState = false;
    }
}
bool MRoomServer::checkProjection()
{
   if(projectionState == false)
       return false;

   int flag = true;
   for(int i = 0; i < userInfoList.count(); i++)
   {
       qDebug() << "checkProjection" << userInfoList.at(i)->pCount;
       if(userInfoList.at(i)->pCount != packetSum)
           flag = false;
   }
   if(flag)
   {
       pIndex += packetSum;
       for(int j = 0; j < userInfoList.count(); j++)
       {
           userInfoList.at(j)->pCount = 0;
       }
   }
   openProjection();
   QTime dieTime = QTime::currentTime().addMSecs(1);
   while( QTime::currentTime() < dieTime )
       QCoreApplication::processEvents(QEventLoop::AllEvents, 100);  //不断刷新，定时，将权限交给其他线程先处理

   return true;
}

bool MRoomServer::openProjection()
{
    if(projectionState == false)
        return false;

    QPixmap pix =  hBScr->getHBitmap();
    QBuffer pixBuf;
    pix.save(&pixBuf,"jpg");
    QByteArray pixByteArray;
    pixByteArray.append(pixBuf.data());
    char *pixChar = pixByteArray.data();

    char *sendBuf = new char[1024];
    int imageSize = pixByteArray.size();
    int total = 0;  //需传送的帧的个数
    int count = 0;
    int lastSize = imageSize % 996;

    //根据最后一个帧是否除尽，判断总的帧数
    if (lastSize == 0)
        total = imageSize / 996;
    else
        total = imageSize / 996+1;

    while (count < total)
    {
        memset(sendBuf, 0, 1024);
        ImageFrameHead frame;
        frame.funCode = 0;
        frame.uTransFrameTotalSize = sizeof(ImageFrameHead);

        //当前发送帧的大小
        if((count+1) != total)
            frame.uTransFrameCurrSize = 996;
        else
            frame.uTransFrameCurrSize = lastSize;

        frame.uDataFrameSize = imageSize;
        frame.uDataFrametotal = total;
        frame.uDataFrameCurr = count+1;
        frame.uDataInFrameOffset = count*(1024 - sizeof(ImageFrameHead));  //此处记录为上一个已发送的偏移量

        memcpy(sendBuf+sizeof(ImageFrameHead), pixChar+frame.uDataInFrameOffset,1024-sizeof(ImageFrameHead));
        memcpy(sendBuf, (char *)&frame, sizeof(ImageFrameHead));  //(char *)&frame:指向frame第一个字节的指针;将frame写入
        for(int i=0; i < userInfoList.count(); i++)
        {
            qDebug() <<userInfoList.at(i)->userIP;
            qDebug() << uDataSocket->write(sendBuf,frame.uTransFrameTotalSize+frame.uTransFrameCurrSize,QHostAddress(userInfoList.at(i)->userIP));
        }
        QTime dieTime = QTime::currentTime().addMSecs(1);
        while( QTime::currentTime() < dieTime )
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);  //不断刷新，定时，将权限交给其他线程先处理
        count++;
    }
    return true;
/* ...投影*/

/* [6]监视*/

}
//开启/关闭监视

void MRoomServer::monitor()
{
    if(monitorState == false)
    {
        monitorAction->setText("关闭监视");
        sendConMsg("Teacher", "CON_MONITOR", QHostAddress("224.2.2.2"));
        monitorState = true;
    }
    else
    {
        monitorAction->setText("开启监视");
        sendConMsg("Teacher", "CON_STOP_MONITOR", QHostAddress("224.2.2.2"));
        monitorState = false;
    }
}

void MRoomServer::showMonitor(QString nameShowed,char *buf, int length)
{
    qDebug() << "showMonitor" <<nameShowed;
    for(int i=0; i<mUserWidgetList.count(); i++)
    {
        MUserWidget *item = mUserWidgetList.at(i);
        if(QString::compare(item->nameLine->text(),nameShowed) == 0)
        {
            QPixmap pixmap;
            pixmap.loadFromData((uchar*)buf, length, "JPG");
            QPixmap fitPixmap = pixmap.scaled(item->mLabelW,item->mLabelH, Qt::KeepAspectRatio, Qt::SmoothTransformation);  // 按比例缩放
            item->mLabel->setPixmap(fitPixmap);
        }
    }
}

//创建监视小控件
void MRoomServer::createMonitorLabel(QString monitorName, int column)
{
    MUserWidget *mUserWidget = new MUserWidget();
    mUserWidget->mWidget = new QWidget();
    mUserWidget->mWidget->setFixedSize(mUserWidget->mLabelW,mUserWidget->mLabelH);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;

    mUserWidget->nameLine = new QLineEdit(monitorName);
    mUserWidget->nameLine->setDisabled(true);
    mUserWidget->nameLine->setAlignment(Qt::AlignHCenter);

    mUserWidget->mLabel = new QLabel();
    mUserWidget->mLabel->setAlignment(Qt::AlignTop);
    mUserWidget->mLabel->resize(mUserWidget->mLabelW,mUserWidget->mLabelH);

//    mUserWidget->mWidget->setStyleSheet("border:2px solid red;");
//    mUserWidget->nameLine->setStyleSheet("border:2px solid red;");
//    mUserWidget->mLabel->setStyleSheet("border:2px solid red;");
    vBoxLayout->addWidget(mUserWidget->nameLine);
    vBoxLayout->addWidget(mUserWidget->mLabel);
    mUserWidget->mWidget->setLayout(vBoxLayout);

    scrollGridLayout->addWidget(mUserWidget->mWidget,mUserWidgetList.count()/column,mUserWidgetList.count()%column,1,1,Qt::AlignLeft|Qt::AlignTop);
    mUserWidgetList.append(mUserWidget);
}
/* ...监视*/

/* [00]工具*/


//搜索指定的UserInfo
PUserInfo MRoomServer::searchUsers(QString userName)
{
    for(int i=0; i<userInfoList.count(); i++)
    {
        if(userInfoList.at(i)->userName == userName)
            return userInfoList.at(i);
    }
    return NULL;
}

/* ...工具*/
//背景绘图
void MRoomServer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

//    QPainter p(this);
//    p.setPen(Qt::NoPen);
//    p.setBrush(Qt::white);
//    p.drawRect(rect());
}



void MRoomServer::setIP()
{
    QSettings*  m_IniFile = new QSettings("Config.ini", QSettings::IniFormat);
    sIP = m_IniFile->value("IP").toString();
    qDebug() << "read" << sIP;
    QList<QNetworkInterface> list;
    QList<QNetworkAddressEntry> list_addrs;
    list = QNetworkInterface::allInterfaces(); //获取系统里所有的网卡对象
    bool flag = false;
    for (int i = 0; i < list.size(); i++)
    {
        intf = list.at(i);
        QNetworkInterface::InterfaceFlags flags = intf.flags();
        //找出处在执行状态，能支持组播的网卡对象
        if ((flags & QNetworkInterface::IsRunning) && (flags & QNetworkInterface::CanMulticast))
        {
            list_addrs = intf.addressEntries(); // 一个网卡可以设多个地址，获取当前网卡对象的所有ip地址

            foreach (QNetworkAddressEntry entry, intf.addressEntries())
            {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    if(sIP == "" || (QString::compare(list_addrs.at(list_addrs.size()-1).ip().toString(), sIP)==0))
                    {
                        flag = true;
                        sIP = list_addrs.at(list_addrs.size()-1).ip().toString();
                        qDebug() << sIP;
                        break;
                    }
                }
            }
        }
        if(flag)
            break;
    }
}

void MRoomServer::on_userListWidget_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* currItem = ui->userListWidget->itemAt(pos);
    remoteControlUser = currItem->text();
    if(currItem->text() == "Teacher")
        return;

    if(currItem == NULL)
        return;

    QMenu *popMenu = new QMenu(this);
    QAction *remoteControlAction = new QAction(tr("学生控制"), this);

    popMenu->addAction(remoteControlAction);

    connect(remoteControlAction, SIGNAL(triggered()), this, SLOT(remoteControl()));
    popMenu->exec(QCursor::pos());

    delete popMenu;
    delete remoteControlAction;
}

void MRoomServer::remoteControl()
{
    //发送远程控制消息
    UserInfo *currUser = searchUsers(remoteControlUser);
    qDebug() << remoteControlUser;
    sendConMsg("Teacher", "CON_REMOTECONTROL", QHostAddress(currUser->userIP));
    remoteControlDlg = new RemoteControlDlg(remoteControlUser, currUser->userIP, controlPort);
    remoteControlDlg->setSocket(controlSocket);
    remoteControlDlg->showMaximized();

}

void MRoomServer::showRemoteControl(char *buf, int length)
{
    remoteControlDlg->showRemoteControl(buf,length);
}

