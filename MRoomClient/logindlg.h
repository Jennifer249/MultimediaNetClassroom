#ifndef LOGINDLG_H
#define LOGINDLG_H

#include <QDialog>
#include "mroomclient.h"
namespace Ui {
class LoginDlg;
}

class LoginDlg : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDlg(QWidget *parent = 0);
    ~LoginDlg();
protected:
    void paintEvent(QPaintEvent *event);
private slots:
    void on_loginBtn_clicked();
    void on_cancelBtn_clicked();
    void loginOK();
    void loginNO();

private:
    void init();
private:
    Ui::LoginDlg *ui;
    MRoomClient* m;
    QString tempUserName;
};

#endif // LOGINDLG_H
