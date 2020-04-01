#include "mroomclient.h"
#include "ui_mroomclient.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QCloseEvent>
#include <QRegExp>
#include <QStringList>
#include <QNetworkInterface>
#include <QBuffer>
#include <QSettings>
#include <QScreen>
#include <windows.h>
#include <QDesktopWidget>
#include <QKeyEvent>
MRoomClient::MRoomClient(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MRoomClient)
{
    ui->setupUi(this);
    controlPort = 16660;
    dataPort = 16661;
    userInfo = new UserInfo();
    userCount = 0;
    groupChatState = true;
    loginOKState = false;
    loginNOState = false;
    conConnectState = false;
    heartTimer = new QTimer();
    mTimer = new QTimer();
    rTimer = new QTimer();
    winW = this->width();
    winH = this->height();
    connControlPort();
}

MRoomClient::~MRoomClient()
{
    delete ui;
}

/* [0]初始化*/

//界面初始化
void MRoomClient::init()
{
    //托盘创建
    QIcon icon(":/images/sysTray.ico");
    systemTray = new QSystemTrayIcon(this);
    systemTray->setIcon(icon);
    systemTray->setToolTip(tr("多媒体教学平台"));
    chatAct = new QAction(tr("聊天"),this);
    connect(chatAct,SIGNAL(triggered()),this, SLOT(showWindows()));
    quitAct = new QAction("Quit Application",this);
    connect(quitAct,SIGNAL(triggered()), this, SLOT(sendQuit()));
    pContextMenu = new QMenu(this);
    pContextMenu->addAction(chatAct);
    pContextMenu->addSeparator();
    pContextMenu->addAction(quitAct);
    systemTray->setContextMenu(pContextMenu);
    systemTray->show();

    //聊天界面操作绑定
    connect(ui->closeBtn,SIGNAL(clicked(bool)),this, SLOT(close()));
    connect(ui->toolComboBox,SIGNAL(activated(int)),this, SLOT(tools(int)));
    connect(ui->sendToComboBox,SIGNAL(activated(int)),this,SLOT(privateChatChange(int)));
}

//获取并验证用户名
bool MRoomClient::setUserName(QString userName)
{
    userInfo->userName = userName;
    sendConMsg("CON_ASKIP",QHostAddress("224.2.2.2"));   //寻找服务器
    setWindowTitle(userName);
    return conConnectState;
}

void MRoomClient::connDataPort()
{
    tDataSocket = new TDataSocket(userInfo->serverIP, dataPort);
    connect(tDataSocket, SIGNAL(recvFileOK()), this, SLOT(recvFileOK()));
    connect(tDataSocket,SIGNAL(connected()),this,SLOT(connectOK()));
    connect(tDataSocket,SIGNAL(disconnected()),qApp,SLOT(quit()));
    connect(tDataSocket, SIGNAL(dealChatMsg(int, QString, QString, QString)), this, SLOT(dealChatMsg(int,QString,QString,QString)));

    uDataSocket = new UDataSocket(dataPort, intf, cIP);
    uDataSocket->start();
    connect(uDataSocket,SIGNAL(recvProjectionOk(char*,unsigned int)),this,SLOT(showProjection(char*,unsigned int)));
    connect(uDataSocket,SIGNAL(recvPacket(int)),this,SLOT(recvPacket(int)));
    heartTimer->start(40000);
    connect(heartTimer, SIGNAL(timeout()), this, SLOT(checkHeart()));
}

void MRoomClient::connectOK()
{
    qDebug() << "tcpSocket connectOK";
    writeChatMsg(UserEnter, userInfo->userName);
}
//...登录部分

// [1]主控端口
//主控线程:UDP方式创建
void MRoomClient::connControlPort()
{

    setIP();
    controlSocket = new QUdpSocket;
    if(!controlSocket->bind(QHostAddress(cIP), controlPort,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint))
    {
        conConnectState = true;
        QMessageBox::critical(this, "error", "bind failed");
        exit(0);
    }
    controlSocket->setMulticastInterface(intf);
    qDebug() << intf.name() <<cIP;
    controlSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,1024*1024*8);
    if (!controlSocket->joinMulticastGroup(QHostAddress("224.2.2.2")))
    {
        QMessageBox::critical(this, "error", "join multicast group failed");
        exit(0);
    }
    connect(controlSocket, SIGNAL(readyRead()),this,SLOT(recvConMsg()));
}

//通过数据端口,发送数据
void MRoomClient::sendConMsg(QString type, QHostAddress serAddr)
{
    QByteArray sData;
    QDataStream out(&sData, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << type;
    out << userInfo->userName;
    qDebug() << "send" << "type" << type << "serAddr" << serAddr;
    controlSocket->writeDatagram(sData, sData.length(), serAddr, controlPort);  //发送给服务器地址
}
//通过数据端口,接收数据
void MRoomClient::recvConMsg()
{
    while (controlSocket->hasPendingDatagrams())
    {
        QByteArray rdata;
        QHostAddress serAddr;
        rdata.resize(controlSocket->pendingDatagramSize());
        controlSocket->readDatagram(rdata.data(),rdata.size(),&serAddr);   //接收数据写入rdata
        QDataStream in(&rdata, QIODevice::ReadOnly);
        QString recvName;
        QString type;
        in >> recvName;

        if(recvName != userInfo->userName && recvName != "Teacher")
            return;
        in >> type;
//        qDebug() <<"1、wait" <<"recvName" << recvName <<"userInfo->userName"<< userInfo->userName<<"serAddr" << serAddr;

        if(type == "CON_LOGINOK")
        {
            if(loginOKState == false)  //UDP将所有网卡广播，导致本地连接时收到多个CON_LOGINOK，也导致tcp连接被创建多个
            {
                emit loginOK();
                QString sIP;
                in >> sIP;
                userInfo->serverIP = QHostAddress(sIP);
                connDataPort();
                qDebug() <<"2、OK" <<"recvName" << recvName <<"userInfo->userName"<< userInfo->userName <<"type" <<type <<"serAddr" << serAddr;
            }
            loginOKState = true;
        }
        else if(type == "CON_LOGINNO")
        {
            if(loginOKState == true)
                return;

            if(loginNOState == false)
            {
                emit loginNO();
                loginNOState = true;
            }
        }
        else if(type == "CON_PROJECTION")
        {
            createProjectionWidget();
        }
        else if(type == "CON_STOP_PROJECTION")
        {
            if(projectionWidget == NULL)
                return;
            projectionWidget->setWindowFlag(Qt::SubWindow);
            projectionWidget->close();
        }
        else if(type == "CON_HEART")
        {
            userInfo->lastTime = QDateTime::currentDateTime();
        }
        else if(type == "CON_MONITOR")
        {
            mTimer->start(100);
            connect(mTimer,SIGNAL(timeout()),this,SLOT(monitor()));
        }
        else if(type == "CON_STOP_MONITOR")
        {
            mTimer->stop();
        }
        else if(type == "CON_REMOTECONTROL")
        {
            rTimer->start(100);
            connect(rTimer,SIGNAL(timeout()),this,SLOT(remoteControl()));
        }
        else if(type == "CON_STOP_REMOTECONTROL")
        {
            rTimer->stop();
        }
        else if(type == "CON_REMOTEMSG")
        {
            QString mouseType;
            QString msga;
            QString msgb;
            in >> mouseType >> msga >> msgb;
            qDebug() << mouseType << msga << msgb;
            if(mouseType == "WINSIZE")
            {
                winW = msga.toInt();
                winH = msgb.toInt();
            }
            if(mouseType == "POI_LPRESS" || mouseType == "POI_RPRESS")
            {
                if(mouseType == "POI_LPRESS")
                        mousepress(true,msga.toInt(),msgb.toInt());
                else
                        mousepress(false,msga.toInt(),msgb.toInt());
            }
            if(mouseType == "POI_MOVE")
            {
                 mousemove(msga.toInt(),msgb.toInt());
            }
            if(mouseType == "POI_LRELEASE" || mouseType == "POI_RRELEASE")
            {
                if(mouseType == "POI_LRELEASE")
                        mouserelease(true,msga.toInt(),msgb.toInt());
                else
                        mouserelease(false,msga.toInt(),msgb.toInt());
            }
            if(type=="POI_DCLICK")
            {
                mousedoubleclick(msga.toInt(),msgb.toInt());
            }
            if(mouseType == "KEY_PRESS" || mouseType == "KEY_RELEASE")
            {
                if(mouseType=="KEY_PRESS")
                        keybord(true,msga.toInt(),msgb);
                 else
                        keybord(false,msga.toInt(),msgb);
            }
        }
    }
}

void MRoomClient::checkHeart()
{
    if (userInfo->lastTime.secsTo(QDateTime::currentDateTime()) >= 30)   //超过30s即为掉线,停止心跳
    {
        heartTimer->stop();
        qDebug() << "heartbeat 超时, 即将断开连接";
        tDataSocket->disconnectFromHost();	//断开连接后会发出信号
        controlSocket->disconnect();
        uDataSocket->leaveMGroup();
        uDataSocket->quit();
        uDataSocket->wait();
        qApp->quit();
    }
}

void MRoomClient::setLoginNOState()
{
    loginNOState = false;
}
//... 主控端口

// [2]聊天
void MRoomClient::on_sendBtn_clicked()
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
    if (ui->msgTextEdit->toPlainText() == "") {
        QMessageBox::warning(0,tr("警告"),tr("发送内容不能为空"),QMessageBox::Ok);
        return;
    }
    ui->msgTextBrowser->verticalScrollBar()->setValue(ui->msgTextBrowser->verticalScrollBar()->maximum());


    if(groupChatState == true)
    {
        writeChatMsg(Msg,ui->msgTextEdit->toPlainText());
    }
    else
    {
        writeChatMsg(PrivateChat,ui->msgTextEdit->toPlainText());
        showPrivateChat(ui->msgTextEdit->toPlainText(), time);
    }

    ui->msgTextEdit->clear();
}

qint64 MRoomClient::writeChatMsg(MsgType type, QString msg)
{
    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);
    //out.setByteOrder(QDataStream::BigEndian);
    out << ushort(0) << quint64(0) << type << msg;
    if(type == (MsgType)0)
    {
        qDebug() <<"cIP" <<cIP ;
        out << cIP;
    }
    out.device()->seek(0);
    ushort CMD = 0;  //聊天
    quint64 len = (quint64)block.size();
    out << CMD << len;
    return tDataSocket->write(block);
}

void MRoomClient::dealChatMsg(int type, QString userName, QString msg, QString time)
{
    switch ((MsgType)type) {
    case Msg:
        updateMsgTextBrowser(userName, msg, time);
        break;
    case UserEnter:
        userEnter(userName);
        break;
    case UserLeft:
        userLeft(userName, time);
        break;
    case UserList:
        updateUserList(msg);
        break;
    case PrivateChat:
        showPrivateChat(msg, time);
    default:
        break;
    }
}

void MRoomClient::updateUserList(QString msg)
{
    QStringList sections = msg.split(QRegExp("[;]"));
    foreach(QString username, sections)
    {
        ui->userListWidget->addItem(username);
        userCount++;qDebug() << "username"<<username <<userCount;
    }
    changeUserCount();
}

void MRoomClient::userEnter(QString username)
{
    bool isEmpty = ui->userListWidget->findItems(username, Qt::MatchExactly).isEmpty();
    if(isEmpty)
    {
        ui->userListWidget->addItem(username);
        ui->msgTextBrowser->setTextColor(Qt::gray);
        ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",10));
        ui->msgTextBrowser->append(tr("%1 在线！").arg(username));
    }
    userCount++;
    changeUserCount();
}

void MRoomClient::userLeft(QString username, QString time)
{
    //删除在线列表离开的用户
     for(int i=0; i< ui->userListWidget->count(); i++)
     {
         if(username == ui->userListWidget->item(i)->text())
         {
             ui->userListWidget->removeItemWidget(ui->userListWidget->takeItem(i));
         }
     }

     ui->msgTextBrowser->setTextColor(Qt::gray);
     ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman", 10));
     ui->msgTextBrowser->append(tr("%1 于 %2 离开！").arg(username).arg(time));
     userCount--;
     changeUserCount();
}

void MRoomClient::changeUserCount()
{
    ui->userNums->setText(tr("在线用户(%1)").arg(userCount));
}

void MRoomClient::updateMsgTextBrowser(QString username, QString msg, QString time)
{
    qDebug() << "hello";
    ui->msgTextBrowser->setTextColor(Qt::blue);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",10));
    ui->msgTextBrowser->append("[ " +username+" ] "+ time);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",9));
    ui->msgTextBrowser->setTextColor(Qt::black);
    ui->msgTextBrowser->append(msg);
}

void MRoomClient::showPrivateChat(QString msg, QString time)
{
    ui->msgTextBrowser->setTextColor(Qt::blue);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",10));
    ui->msgTextBrowser->append("私聊：[ Teacher ] "+ time);
    ui->msgTextBrowser->setCurrentFont(QFont("Times New Roman",9));
    ui->msgTextBrowser->setTextColor(Qt::black);
    ui->msgTextBrowser->append(msg);
}
//...聊天

void MRoomClient::closeEvent(QCloseEvent *e)
{
    e->ignore();
    this->hide();
}

void MRoomClient::sendQuit()
{
//    writeChatMsg(UserLeft,userInfo->userName);
    tDataSocket->disconnectFromHost();	//断开连接后会发出信号
    uDataSocket->quit();
    uDataSocket->wait();
}

void MRoomClient::sendMsg(MsgType type, QString msg)
{
    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);
    out << (quint64)0;
    out << type;
    out << msg;
    out.device()->seek(0);
    out << (quint64) (block.size() - sizeof(quint64));
    tDataSocket->write(block);
}

//私聊
void MRoomClient::privateChatChange(int index)
{
    switch (index) {
    case 0:
        groupChatState = true;
        break;
    case 1:
        groupChatState = false;
        break;
    default:
        break;
    }
}

//触发工具
void MRoomClient::tools(int index)
{
    switch (index) {
    case 1:
        sFile = new SFile(tDataSocket);
        sFile->setAttribute(Qt::WA_DeleteOnClose);
        sFile->show();
        break;
    default:
        break;
    }
}


void MRoomClient::showWindows()
{
    this->show();
}

void MRoomClient::recvFileOK()
{
    QMessageBox::warning(0,tr("文件传输"),tr("文件已接收"),QMessageBox::Ok);
}


/* [5]投影*/

void MRoomClient::createProjectionWidget()
{
    //设置布局
    projectionWidget = new QWidget(NULL, Qt::Window);
    vBoxLayout = new QVBoxLayout(projectionWidget);
    projectionWidget->showFullScreen();
    projectionLabel = new QLabel(projectionWidget);
    vBoxLayout->addWidget(projectionLabel);

    //设置背景色
    QPalette palette(this->palette());
    palette.setColor(QPalette::Background, Qt::black);
    projectionWidget->setPalette(palette);
    projectionLabel->setAutoFillBackground(true);
    projectionLabel->setPalette(palette);
}

void MRoomClient::showProjection(char *buf, unsigned int length)
{
    //获取图片
    QPixmap pixmap;
    qDebug() << "bug is here1";
    pixmap.loadFromData((uchar*)buf, length, "JPG");
    qDebug() << "bug is here2";

    //展示图片
    int width = projectionLabel->width()*0.9;
    int height = projectionLabel->height()*0.9;
    QPixmap fitPixmap = pixmap.scaled(width,height,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    projectionLabel->setPixmap(fitPixmap);
    projectionLabel->setAlignment(Qt::AlignCenter);
}

void MRoomClient::recvPacket(int pCount)
{
    QByteArray sData;
    QDataStream out(&sData, QIODevice::WriteOnly);
    QString type = "PRO_OK";
    out.setByteOrder(QDataStream::BigEndian);
    out << type;
    out << userInfo->userName;
    out << pCount;
    if((controlSocket->writeDatagram(sData, sData.length(), QHostAddress("224.2.2.2"), controlPort)) <= 0)
    {
        qDebug() <<"write wrong";
    }//发送给服务器地址
}
/* ...投影*/

/* [6]监视*/
void MRoomClient::monitor()
{
    QPixmap pix = hBScr->getHBitmap();
    QBuffer pixBuf;
    pix.save(&pixBuf, "jpg");
    QByteArray pixByteArray;
    pixByteArray.append(pixBuf.data());
    char* pixChar = pixByteArray.data();
    char *sendBuf = new char[1024+68];
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
        memset(sendBuf, 0, 1092);
        ImageFrameHead frame;
        frame.funCode = 1;
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

        ImageUserInfo imageUserInfo;
        imageUserInfo.CMD = 0;  //监视
        imageUserInfo.dataSize = sizeof(ImageFrameHead) + frame.uTransFrameCurrSize;

        memcpy(imageUserInfo.userName, userInfo->userName.toStdString().c_str(),userInfo->userName.toStdString().size());
        memcpy(sendBuf+sizeof(ImageFrameHead)+sizeof(ImageUserInfo), pixChar+frame.uDataInFrameOffset,1092-sizeof(ImageFrameHead)-sizeof(ImageUserInfo));
        memcpy(sendBuf, (char *)&imageUserInfo, sizeof(ImageUserInfo));
        memcpy(sendBuf+sizeof(ImageUserInfo), (char *)&frame, sizeof(ImageFrameHead));  //(char *)&frame:指向frame第一个字节的指针;将frame写入

        if((uDataSocket->write(sendBuf,frame.uTransFrameTotalSize+frame.uTransFrameCurrSize+sizeof(ImageUserInfo),QHostAddress(userInfo->serverIP)))< 0)
                return;

        QTime dieTime = QTime::currentTime().addMSecs(1);
        while( QTime::currentTime() < dieTime )
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);  //不断刷新，定时，将权限交给其他线程先处理
        count++;
    }
}

/* [7]远程控制*/
void MRoomClient::remoteControl()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QBuffer pixBuf;
    screen->grabWindow(0).scaled(winW, winH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).toImage().save(&pixBuf,"JPG");

    QByteArray pixByteArray;
    pixByteArray.append(pixBuf.data());
    char* pixChar = pixByteArray.data();
    char *sendBuf = new char[1024+68];
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
        memset(sendBuf, 0, 1092);
        ImageFrameHead frame;
        frame.funCode = 1;
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

        ImageUserInfo imageUserInfo;
        imageUserInfo.CMD = 1; //远程控制
        imageUserInfo.dataSize = sizeof(ImageFrameHead) + frame.uTransFrameCurrSize;

        memcpy(imageUserInfo.userName, userInfo->userName.toStdString().c_str(),userInfo->userName.toStdString().size());
        memcpy(sendBuf+sizeof(ImageFrameHead)+sizeof(ImageUserInfo), pixChar+frame.uDataInFrameOffset,1092-sizeof(ImageFrameHead)-sizeof(ImageUserInfo));
        memcpy(sendBuf, (char *)&imageUserInfo, sizeof(ImageUserInfo));
        memcpy(sendBuf+sizeof(ImageUserInfo), (char *)&frame, sizeof(ImageFrameHead));  //(char *)&frame:指向frame第一个字节的指针;将frame写入

        if((uDataSocket->write(sendBuf,frame.uTransFrameTotalSize+frame.uTransFrameCurrSize+sizeof(ImageUserInfo),QHostAddress(userInfo->serverIP)))< 0)
                return;

        QTime dieTime = QTime::currentTime().addMSecs(1);
        while( QTime::currentTime() < dieTime )
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);  //不断刷新，定时，将权限交给其他线程先处理
        count++;
    }
}

void MRoomClient::setIP()
{
    QSettings*  m_IniFile = new QSettings("Config.ini", QSettings::IniFormat);
    cIP = m_IniFile->value("IP").toString();
    qDebug() << "read" << cIP;
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
                    if(cIP == "" || (QString::compare(list_addrs.at(list_addrs.size()-1).ip().toString(), cIP)==0))
                    {
                        flag = true;
                        cIP = list_addrs.at(list_addrs.size()-1).ip().toString();
                        qDebug() << cIP;
                        break;
                    }
                }
            }
        }
        if(flag)
            break;
    }
}

void MRoomClient::mousepress(bool LR, int x, int y)
{
#ifdef Q_OS_WIN
    if(LR)
        mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
    else
        mouse_event(MOUSEEVENTF_RIGHTDOWN,0,0,0,0);
#endif
}
void MRoomClient::mousemove(int x, int y)
{
    QDesktopWidget *deskWidget = QApplication::desktop();
    mouse.setPos(deskWidget->width()*x/winW,QApplication::desktop()->height()*y/winH);
}

void MRoomClient::mouserelease(bool LR, int x, int y)
{
#ifdef Q_OS_WIN        //声明要使用windows API
    if(LR) mouse_event (MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    else mouse_event (MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0 );
#endif
}

void MRoomClient::mousedoubleclick(int x, int y)
{
#ifdef Q_OS_WIN        //声明要使用windows API
    mouse.setPos(QApplication::desktop()->width()*x/winW,QApplication::desktop()->height()*y/winH);
    mouse_event (MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    mouse_event (MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
#endif                 //有ifdef，就必须有endif
}

void MRoomClient::keybord(bool PR, int key, QString text)
{
#ifdef Q_OS_WIN        //声明要使用windows API
    if(PR) keybd_event(translateKeyCode(key),0x45, KEYEVENTF_EXTENDEDKEY, 0);
    else keybd_event(translateKeyCode(key),0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
#endif
}

int MRoomClient::translateKeyCode(int key)
{
    int k = key;
    bool legal = true;
    if(k >= Qt::Key_0 && k <= Qt::Key_9)
    {

        qDebug() << "0-9"<<k <<"Qt::Key_0" <<Qt::Key_0;
    }
    else if(k >= Qt::Key_A && k <= Qt::Key_Z)
    {
    }
    else if(k >= Qt::Key_F1 && k <= Qt::Key_F24)
    {
        k &= 0x000000ff;
        k += 0x40;
    }
    else if(k == Qt::Key_Tab)
    {
        k = 0x09;
    }
    else if(k == Qt::Key_Backspace)
    {
        k = 0x08;
    }
    else if(k == Qt::Key_Return)
    {
        k = 0x0d;
    }
    else if(k <= Qt::Key_Down && k >= Qt::Key_Left)
    {
        int off = k - Qt::Key_Left;
        k = 0x25 + off;
    }
    else if(k == Qt::Key_Shift)
    {
        k = 0x10;
    }
    else if(k == Qt::Key_Control)
    {
        k = 0x11;
    }
    else if(k == Qt::Key_Alt)
    {
        k = 0x12;
    }
    else if(k == Qt::Key_Meta)
    {
        k = 0x5b;
    }
    else if(k == Qt::Key_Insert)
    {
        k = 0x2d;
    }
    else if(k == Qt::Key_Delete)
    {
        k = 0x2e;
    }
    else if(k == Qt::Key_Home)
    {
        k = 0x24;
    }
    else if(k == Qt::Key_End)
    {
        k = 0x23;
    }
    else if(k == Qt::Key_PageUp)
    {
        k = 0x21;
    }
    else if(k == Qt::Key_Down)
    {
        k = 0x22;
    }
    else if(k == Qt::Key_CapsLock)
    {
        k = 0x14;
    }
    else if(k == Qt::Key_NumLock)
    {
        k = 0x90;
    }
    else if(k == Qt::Key_Space)
    {
        k = 0x20;
    }
    else
        legal = false;

    if(!legal)
        return 0;
    qDebug() << "change"<<k;
    return k;
}
