#include "sfserver.h"
#include "ui_sfserver.h"
#include <QFileDialog>
#include <QMessageBox>

int userCount = 0;
SFServer::SFServer(quint16 sFilePort,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SFServer)
{
    ui->setupUi(this);
    this->sFilePort = sFilePort;
    tcpServer = new QTcpServer(this);
    connect(tcpServer,SIGNAL(newConnection()),this,SLOT(newUsers()));
    initServerWin();
    //监听
    if(!tcpServer->listen(QHostAddress::Any,sFilePort))
    {
        qDebug() << tcpServer->errorString(); 
        close();
        return;
    }
}
// [0]界面初始化
void SFServer::initServerWin()
{

    ui->msgLabel->setText(tr("请选择要传送的文件"));
    ui->progressBar->reset();
    ui->openBtn->setEnabled(true);
    ui->sendBtn->setEnabled(false);
    userCount = 0;
    tcpServer->close();
}
// [0]界面初始化

void SFServer::closeEvent(QCloseEvent *e)
{
    emit closeThread();
    e->accept();
}
void SFServer::newUsers()
{
    int socketDescriptor = tcpServer->nextPendingConnection()->socketDescriptor();
    SFSocketThread *thread = new SFSocketThread(this,socketDescriptor);
    userCount++;
    thread->start();
    connect(thread,SIGNAL(updateFileWritten(qint64,qint64,QString)),this,SLOT(updateProgressBar(qint64,qint64,QString)));
    connect(thread,SIGNAL(fileWarningToSer(QString)),this,SLOT(fileWarning(QString)));

}
void SFServer::fileWarning(QString error)
{
    QMessageBox::warning(this, tr("应用程序"), tr("无法读取文件 %1:\n%2").arg(fileName).arg(error));
}

void SFServer::updateProgressBar(qint64 totalBytes,qint64 bytesWritten,QString msg)
{
    ui->progressBar->setMaximum(totalBytes*userCount);
    ui->progressBar->setValue(bytesWritten);
    ui->msgLabel->setText(msg);
    if(bytesWritten == totalBytes*userCount)
    {
        ui->msgLabel->setText(tr("传送文件%1成功").arg(theFileName));
        ui->progressBar->setValue(totalBytes);
        userCount = 0;
        bytesWritten = 0;
        tcpServer->close();
    }
}

SFServer::~SFServer()
{
    delete ui;
}

void SFServer::on_sendBtn_clicked()
{
    ui->msgLabel->setText(tr("准备发送文件 %1 ").arg(theFileName));
    ui->sendBtn->setEnabled(false);
    emit sendFile(fileName);
}

void SFServer::on_openBtn_clicked()
{
    fileName = QFileDialog::getOpenFileName(this);
    if(!fileName.isEmpty())
    {
        theFileName = fileName.right(fileName.size()-fileName.lastIndexOf('/')-1); //匹配从右边开始的子项
        ui->msgLabel->setText(tr("要传送的文件为:%1").arg(theFileName));
        ui->sendBtn->setEnabled(true);
        ui->openBtn->setEnabled(false);
    }
}

void SFServer::on_closeBtn_clicked()
{
    if(tcpServer->isListening())
    {
        tcpServer->close();
    }
    close();
}
