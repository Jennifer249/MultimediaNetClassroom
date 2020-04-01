#include "sfile.h"
#include "ui_sfile.h"
#include <QFileDialog>
#include <QMessageBox>

SFile::SFile(QList<PUserInfo> userInfoList,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SFile)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    this->userInfoList = userInfoList;
}

SFile::~SFile()
{
    delete ui;
}

void SFile::on_openBtn_clicked()
{
    filePath = QFileDialog::getOpenFileName(this);
    if(!filePath.isEmpty())
    {
        fileName = filePath.right(filePath.size()-filePath.lastIndexOf('/')-1); //匹配从右边开始的子项
        ui->msgLabel->setText(tr("要传送的文件为:%1").arg(fileName));
        ui->sendBtn->setEnabled(true);
        ui->openBtn->setEnabled(false);
    }
}

void SFile::on_sendBtn_clicked()
{
    writtenSum = 0;
    ui->msgLabel->setText(tr("准备发送文件 %1 ").arg(fileName));
    ui->sendBtn->setEnabled(false);
    for(int i=0; i<userInfoList.count(); i++)
    {
        connect(userInfoList.at(i)->tDataSocket, SIGNAL(fileWarning(QString)),this, SLOT(fileWarning(QString)));
        connect(userInfoList.at(i)->tDataSocket, SIGNAL(updateProgressBar(QString, qint64,qint64,float)),this,SLOT(updateProgressBar(QString, qint64,qint64,float)));
        userInfoList.at(i)->tDataSocket->writeFile(filePath);
    }
}

void SFile::fileWarning(QString error)
{
    QMessageBox::warning(this, tr("应用程序"), tr("无法读取文件 %1:\n%2").arg(fileName).arg(error));
}

void SFile::updateProgressBar(QString userName, qint64 totalBytes, qint64 bytesWritten, float useTime)
{

    qDebug() << totalBytes << bytesWritten << userName <<writtenSum;
    for(int i=0; i<userInfoList.count(); i++)
    {
        if(userInfoList.at(i)->userName == userName)
        {
            writtenSum -= userInfoList.at(i)->recvFileData;
            userInfoList.at(i)->recvFileData = bytesWritten;
            writtenSum += bytesWritten;
        }
    }

    ui->progressBar->setMaximum(totalBytes*userInfoList.count());
    ui->progressBar->setValue(writtenSum);

    double speed = totalBytes*userInfoList.count() / useTime;
    QString msg = tr("已发送%1MB(%2MB/s \n共%3MB 已用时：%4秒\n估计剩余时间：%5s)")
            .arg((double)writtenSum/(1024*1024),0,'f',2)
            .arg(speed*1000/(1024*1024),0,'f',2)
            .arg((double)totalBytes*userInfoList.count()/(1024*1024),0,'f',2)
            .arg(useTime/1000,0,'f',0)
            .arg((double)totalBytes*userInfoList.count()/speed/1000 - useTime/1000,0,'f',0);
    ui->msgLabel->setText(msg);

    qDebug() << "updateProgressBar" << writtenSum << totalBytes*userInfoList.count();
    if(writtenSum == totalBytes*userInfoList.count())
    {
        ui->msgLabel->setText(tr("传送文件%1成功").arg(fileName));
        writtenSum = 0;
        for(int i=0; i<userInfoList.count(); i++)
        {
            userInfoList.at(i)->recvFileData = 0;
        }
    }
}

void SFile::on_closeBtn_clicked()
{
    this->close();
}
