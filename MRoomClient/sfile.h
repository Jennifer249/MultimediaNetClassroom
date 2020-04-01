#ifndef SFILE_H
#define SFILE_H

#include <QDialog>
#include <QFile>
#include <QTime>
#include "tdatasocket.h"
namespace Ui {
class SFile;
}

class SFile : public QDialog
{
    Q_OBJECT

public:
    explicit SFile(TDataSocket*, QWidget *parent = 0);
    ~SFile();

private slots:
    void on_openBtn_clicked();
    void on_sendBtn_clicked();
    void fileWarning(QString);
    void updateProgressBar(qint64,qint64,QString);

private:
    Ui::SFile *ui;
    QString filePath;
    QString fileName;
    TDataSocket* tDataSocket;

    qint64 totalBytes;      //总共
    qint64 bytesWritten;    //已经写的
    qint64 bytesTobeWrite;  //剩下的
    qint64 payloadSize;
    QTime time;
    qint64 writtenSum;
};

#endif // SFILE_H
