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
}

LoginDlg::~LoginDlg()
{
    delete ui;
}

//初始化
void LoginDlg::init()
{
    setFixedSize(300,200);
    setWindowTitle(tr("局域网多媒体教学软件"));
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    ui->pwEdit ->setEchoMode(QLineEdit::Password);
}

//背景绘图
void LoginDlg::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRect(rect());
}

//点击登录按钮
void LoginDlg::on_loginBtn_clicked()
{
    if(ui->IDEdit->text() == "teacher" && ui->pwEdit->text() == "123456")
    {
        MRoomServer* m =  new MRoomServer();
        m->setAttribute(Qt::WA_DeleteOnClose);  //除了隐藏窗口，还会释放这个窗口所占的资源
        m->show();
        this->close();
    }
    else
    {
        QMessageBox::warning(this,tr("登录失败"),tr("账号或密码错误"));
    }
}

//点击取消按钮
void LoginDlg::on_cancelBtn_clicked()
{
    this->close();
}
