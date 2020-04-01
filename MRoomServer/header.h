#ifndef HEADER_H
#define HEADER_H
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QHostAddress>
#include <tdatasocket.h>
#include "string.h"
struct ImageFrameHead
{
   int funCode;
   unsigned int uTransFrameTotalSize;  //数据帧总大小
   unsigned int uTransFrameCurrSize;  //数据现在的大小

   //数据帧变量
   unsigned int uDataFrameSize;  //文件的大小
   unsigned int uDataFrametotal;  //数据帧总的个数
   unsigned int uDataFrameCurr;  //当前第几个帧
   unsigned int uDataInFrameOffset;  //偏移量
};

struct ImageUserInfo  //typdef 用新的变量类型去定义新的变量
{
    char userName[60];
    unsigned int CMD;
    unsigned int dataSize;   //头部后面的数据内容的大小
    ImageUserInfo()
    {
        memset(this, 0, sizeof(ImageUserInfo));
    }
};

struct MUserWidget
{
    QWidget *mWidget;
    QLineEdit *nameLine;
    QLabel *mLabel;
    int mLabelW;
    int mLabelH;
    MUserWidget()
    {
        mLabelW = 450;
        mLabelH = 450;
        mWidget = NULL;
        nameLine = NULL;
        mLabel = NULL;
    }
    void setWH(int w, int h)
    {
        mLabelW = w;
        mLabelH = h;
    }
};

typedef struct tagUserInfo
{
    QString userName;
    QString userIP;
    qint64 recvFileData;
    bool chatState;
    bool monitorState;
    bool projectionState;
    TDataSocket *tDataSocket;
    int pCount;
    tagUserInfo()
    {
        userName = "";
        userIP = "" ;
        recvFileData = 0;
        chatState = false;
        monitorState = false;
        projectionState = false;
        tDataSocket = NULL;
        pCount = 0;
    }
}UserInfo, *PUserInfo;


#endif // HEADER_H
