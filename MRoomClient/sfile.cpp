#include "sfile.h"
#include "ui_sfile.h"
#include <QFileDialog>
#include <QMessageBox>

SFile::SFile(TDataSocket* tDataSocket,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SFile)
{
    ui->setupUi(this);
    this->tDataSocket = tDataSocket;
    connect(ui->closeBtn, SIGNAL(clicked()), this, SLOT(close()));
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
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
    tDataSocket->writeFile(filePath);
    connect(tDataSocket, SIGNAL(fileWarning(QString)),this, SLOT(fileWarning(QString)));
    connect(tDataSocket, SIGNAL(updateProgressBar(qint64,qint64,QString)),this,SLOT(updateProgressBar(qint64,qint64,QString)));
}

void SFile::fileWarning(QString error)
{
    QMessageBox::warning(this, tr("应用程序"), tr("无法读取文件 %1:\n%2").arg(fileName).arg(error));
}

void SFile::updateProgressBar(qint64 totalBytes, qint64 bytesWritten, QString msg)
{
    ui->progressBar->setMaximum(totalBytes);
    ui->progressBar->setValue(bytesWritten);
    ui->msgLabel->setText(msg);
    qDebug() << "updateProgressBar" << bytesWritten << totalBytes;
    if(bytesWritten == totalBytes)
    {
        ui->msgLabel->setText(tr("传送文件%1成功").arg(fileName));
        bytesWritten = 0;
    }
}
