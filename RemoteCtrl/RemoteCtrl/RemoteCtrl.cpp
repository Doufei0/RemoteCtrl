// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "LockInfoDialog.h"
#include <direct.h>
#include <iostream>
#include <io.h>
#include <stdio.h>
#include <list>
#include <atlimage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CLockInfoDialog dlg;
unsigned threadId = 0;


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize) // 打印出数据查看核对
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0))
            strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    //printf("%s \n", strOut.c_str());
    OutputDebugStringA(strOut.c_str());
}


int MakeDriverInfo() {
    string result;
    for (int i = 1; i <= 26; ++i)
    {
        if (_chdrive(i) == 0)
        {
            if (result.size() > 0)
            {
                result += ',';
            }
            result += 'A' + i - 1; // result 就是拿到的磁盘符号
        }
    }
    result += ','; // 最后也跟上一个逗号分隔
    CPackage pack(1, (BYTE*)result.c_str(), result.size()); //打包数据的构造函数重载
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);

    return 0;
}

int MakeDirectoryInfo() {
    string strPath;
    //list<FILEINFO> listFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false)
    {
        OutputDebugString(_T("当前的命令不是获取文件列表，逻辑错误，命令解析错误！"));
        return -1;
    }

    if (_chdir(strPath.c_str()) != 0)
    {
        FILEINFO finfo;
        //finfo.IsInvalid = TRUE;
        //finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        //memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        //listFileInfos.push_back(finfo);  // 不用列表的形式，而是读到一个文件就发一个文件
        // 随读 随发
        CPackage pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限，访问目录！"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1)
    {
        OutputDebugString(_T("没有找到任何文件！"));
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPackage pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return -3;
    }
    // 上面是各种文件查找出错的情况
    // 下面是循环查找文件夹目录下的内容
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        TRACE("%s\r\n", finfo.szFileName);
        CPackage pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));

    // 最后发一个空的数据包
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPackage pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);

    return 0;
}

int RunFile() {
    string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CPackage pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DownloadFile() {
    string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0)
    {
        CPackage pack(4, (BYTE*)data, 8); // 8代表8字节长度，longlong类型占用8字节
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile == NULL)
    {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPackage head(4, (BYTE*)data, 8);
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do
        {
            rlen = fread(buffer, 1, 1024, pFile);
            CPackage pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024); // 读到文件尾了就退出循环
        fclose(pFile);
    }

    // 最后发一个空过去，代表读完了发送结束
    CPackage pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse))
    {
        DWORD nFlags = 0;
        switch (mouse.nButton)
        {
        case 0:  // 左键
            nFlags = 1;
            break;
        case 1:  // 右键
            nFlags = 2;
            break;
        case 2:  // 中键
            nFlags = 4;
            break;
        case 4:  // 没按键，只移动
            nFlags = 8;
            break;
        }
        if (nFlags != 8)
        {
            SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        }
        switch (mouse.nAction)
        {
        case 0: // 单击
            nFlags |= 0x10;
            break;
        case 1: // 双击
            nFlags |= 0x20;
            break;
        case 2: // 按下
            nFlags |= 0x40;
            break;
        case 3: // 松开
            nFlags |= 0x80;
            break;
        default:
            break;
        }
        switch (nFlags)
        {
        case 0x21:  // 左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11:  // 左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41:  // 左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81:  // 左键松开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x22:  // 右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12:  // 右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42:  // 右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82:  // 右键松开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x24:  // 中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14:  // 中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44:  // 中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84:  // 中键松开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x08:  // 单纯移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPackage pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else
    {
        OutputDebugString(_T("获取鼠标操作参数失败！"));
        return -1;
    }


    return 0;
}

int SendScreen() {
    CImage screen;
    HDC hScreen = ::GetDC(NULL);
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL); // 24 位的色彩
    int nWidth = GetDeviceCaps(hScreen, HORZRES);
    int nHeight = GetDeviceCaps(hScreen, VERTRES);
    screen.Create(nWidth, nHeight, nBitPerPixel);
    BitBlt(screen.GetDC(), 0, 0, 2560, 1600, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);
    HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMem == NULL)
        return -1;
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (ret == S_OK)
    {
        // 把截图文件保存到内存流中，而不是保存到本地
        screen.Save(pStream, Gdiplus::ImageFormatJPEG);
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData = (PBYTE)GlobalLock(hMem);
        SIZE_T nSize = GlobalSize(hMem);
        CPackage pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);
    }
    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();

    //screen.Save(_T("test.jpg"), Gdiplus::ImageFormatJPEG);
    // 这部分用于测试两种格式的时间开销和空间开销，选择其中一个
    /*DWORD tick = GetTickCount64();
    screen.Save(_T("test.png"), Gdiplus::ImageFormatPNG);
    TRACE("png %d\r\n", GetTickCount64() - tick);
    tick = GetTickCount64();
    screen.Save(_T("test.jpg"), Gdiplus::ImageFormatJPEG);
    TRACE("jpg %d\r\n", GetTickCount64() - tick);*/
    //screen.Save(_T("test.png"), Gdiplus::ImageFormatPNG); // png 的开销更大

    return 0;
}

unsigned _stdcall threadLockDlg(void* arg) {

    dlg.Create(IDD_DIALOG_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);
    // 屏蔽后台窗口
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    dlg.MoveWindow(rect);
    
    // 窗口置顶
    //dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    // 隐藏任务栏
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);

    // 限制鼠标活动范围和功能
    ShowCursor(false);
    dlg.GetWindowRect(rect);
    ClipCursor(rect);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        // 按下按键，结束消息循环
        if (msg.message == WM_KEYDOWN) {
            TRACE("msg:%08X wparam:%08x lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == 0x1B) // 按下 ESC 退出
                break;
        }
    }
    ShowCursor(true);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    dlg.DestroyWindow();
    _endthreadex(0);

    return 0;
}

int LockMachine() {

    if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadId);
    }
    CPackage pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnlockMachine() {
    PostThreadMessage(threadId, WM_KEYDOWN, 0x1B, 0);
    CPackage pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int connectTest() {
    CPackage pack(1999, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int ExcuteCommand(int cmd) {
    int ret = 0;
    switch (cmd)
    {
    case 1: // 查看磁盘分区
        ret = MakeDriverInfo();
        break;
    case 2: // 查看指定目录下的文件
        ret = MakeDirectoryInfo();
        break;
    case 3: // 打开文件
        ret = RunFile();
        break;
    case 4: // 客户端下载文件，指服务器发送（上传文件
        ret = DownloadFile();
        break;
    case 5: // 鼠标操作
        ret = MouseEvent();
    case 6: // 监控并发送屏幕内容-->发送屏幕的截图
        ret = SendScreen();
        break;
    case 7: // 锁机
        ret = LockMachine();
        break;
    case 8: // 解锁
        ret = UnlockMachine();
        break;
    case 1999:
        connectTest();
        break;
    }

    return ret;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            int count = 0;
            CServerSocket* pserver = CServerSocket::getInstance();
            if (pserver->InitSocket() == false)
            {
                MessageBox(NULL, _T("网络初始化异常，未成功初始化，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while(CServerSocket::getInstance() != NULL)
            {
                if (pserver->AcceptClient() == false)
                {
                    if (count >= 3)
                    {
                        MessageBox(NULL, _T("多次无法正常接入用户，直接退出"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    count++;
                }

                TRACE("AcceptClient return true\r\n");
                int ret = pserver->DealCommand();
                TRACE("Deal command ret: %d\r\n", ret);

                if (ret > 0)
                {   // 成功返回则执行对应命令
                    ret = ExcuteCommand(pserver->GetPackage().sCmd);
                    if (ret != 0)
                    {
                        TRACE("执行命令失败：%d ret = %d\r\n", pserver->GetPackage(), ret);
                    }
                    // 本例使用短连接，即连接成功后收发一次即断开连接
                    pserver->CloseClient();
                }
            }

            
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
