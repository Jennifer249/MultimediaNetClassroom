#include "HBScreenShot.h"
#include <windows.h>
#include "QtWinExtras/qwinfunctions.h"
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
HBScreenShot::HBScreenShot()
{

}


HBScreenShot::~HBScreenShot()
{
}

QPixmap HBScreenShot::getHBitmap()
{
    HWND hWnd = GetDesktopWindow();  //返回桌面窗口句柄
    RECT re;  //创建一个矩形对象
    GetWindowRect(hWnd, &re);  //获取窗口大小

    int rWidth = re.right - re.left;  //获取窗口的宽
    int rHeight = re.bottom - re.top;  //获取窗口的长
    HDC hScrDC = GetWindowDC(hWnd);  //返回hWnd参数所指定的窗口的设备环境
    HDC hMemDC = CreateCompatibleDC(hScrDC);  //创建一个兼容的内存画板

    HBITMAP hBitmap = CreateCompatibleBitmap(hScrDC, rWidth, rHeight);  //该函数用于创建与指定的设备环境相关的设备兼容的位图
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);  //选中画笔
    BitBlt(hMemDC, 0, 0, rWidth, rHeight, hScrDC, 0, 0, SRCCOPY);  //绘制图像

    //获取鼠标位置
    POINT po;
    GetCursorPos(&po);

    //获取鼠标信息,使用GetCursorInfo函数获取光标信息，其中包含图标
    CURSORINFO hCur;
    ZeroMemory(&hCur, sizeof(hCur));
    hCur.cbSize = sizeof(hCur);
    GetCursorInfo(&hCur);

    //添加鼠标图像
    HICON hinco = hCur.hCursor;
    DrawIcon(hMemDC,po.x - 10, po.y - 10, hinco);

    //恢复原来的画笔
    hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);
    return qt_pixmapFromWinHBITMAP(hBitmap);
}
