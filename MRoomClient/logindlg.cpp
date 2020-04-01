#include "logindlg.h"
#include "ui_logindlg.h"
#include <QPainter>
#include <QMessageBox>
LoginDlg::LoginDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDlg)
{
    ui->setupUi(this);
    init();
    m =  new MRoomClient();
    tempUserName = "";
}

LoginDlg::~LoginDlg()
{
    delete ui;
}

void LoginDlg::init()
{
    setWindowTitle(tr("局域网多媒体教学软件"));
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
}

void LoginDlg::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRect(rect());
}

void LoginDlg::on_loginBtn_clicked()
{
    QString inputNames = tr("%1").arg(ui->IDEdit->text());
    m->setLoginNOState();
    if(inputNames.isEmpty())
    {
        QMessageBox::warning(0,tr("输入错误"),tr("用户名不得为空"),QMessageBox::Ok);
        return;
    }
    if(inputNames == "Teacher")
    {
        QMessageBox::warning(0,tr("输入错误"),tr("用户名不得为教师"),QMessageBox::Ok);
        return;
    }
    if(tempUserName == "" || tempUserName != inputNames)
    {
        qDebug() << tempUserName <<inputNames;
        if(m->setUserName(inputNames))
        {
            tempUserName = inputNames;
        }
    }
    else if(tempUserName == inputNames)
        QMessageBox::warning(0,tr("登录失败"),tr("该用户名已登录不得重复登录"),QMessageBox::Ok);
    connect(m,SIGNAL(loginOK()),this,SLOT(loginOK()));
    connect(m,SIGNAL(loginNO()),this,SLOT(loginNO()));
}

void LoginDlg::on_cancelBtn_clicked()
{
    this->close();
}

void LoginDlg::loginOK()
{
    m->show();
    m->init();
    this->close();
}

void LoginDlg::loginNO()
{
    m->close();
    QMessageBox::warning(0,tr("登录失败"),tr("该用户名已登录不得重复登录"),QMessageBox::Ok);
    qDebug() << "loginON";
}
