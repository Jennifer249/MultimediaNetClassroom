#-------------------------------------------------
#
# Project created by QtCreator 2019-02-26T21:46:02
#
#-------------------------------------------------

QT       += core gui
QT       += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MRoomServer
TEMPLATE = app
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
LIBS += -luser32 -lws2_32 -lgdi32
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mroomserver.cpp \
    logindlg.cpp \
    tdatasocket.cpp \
    sfile.cpp \
    HBScreenShot.cpp \
    monitorthread.cpp \
    udatasocket.cpp \
    remotecontroldlg.cpp

HEADERS += \
        mroomserver.h \
    header.h \
    logindlg.h \
    tdatasocket.h \
    sfile.h \
    HBScreenShot.h \
    monitorthread.h \
    udatasocket.h \
    remotecontroldlg.h

FORMS += \
        mroomserver.ui \
    logindlg.ui \
    sfile.ui \
    remotecontroldlg.ui

RESOURCES += \
    resource.qrc
