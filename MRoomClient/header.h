#ifndef HEADER_H
#define HEADER_H
#include <QString>
#include <QHostAddress>
#include <QDateTime>
typedef struct tagUserInfo
{
    QString userName;
    QHostAddress serverIP;
    bool chatState;
    bool monitorState;
    bool projectionState;
    QDateTime lastTime;
    tagUserInfo()
    {
        userName = "";
        serverIP = QHostAddress::Null ;
        chatState = false;
        monitorState = false;
        projectionState = false;
    }
}UserInfo, *PUserInfo;

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

struct Point
{
    double x;
    double y;
    Point()
    {
        memset(this,0,sizeof(Point));
    }
};

#endif // HEADER_H
