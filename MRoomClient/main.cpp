#include <QApplication>
#include "logindlg.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::addLibraryPath("./dll");
    LoginDlg w;
    w.show();

    return a.exec();
}
