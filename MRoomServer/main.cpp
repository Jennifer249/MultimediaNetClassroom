#include "logindlg.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LoginDlg w;
    w.show();

    return a.exec();
}
