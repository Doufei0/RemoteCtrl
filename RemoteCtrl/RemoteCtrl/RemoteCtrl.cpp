// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <iostream>
#include <io.h>
#include <stdio.h>
#include <list>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

typedef struct file_info{
    file_info (){
        IsInvalid = FALSE;
        IsDirectory = -1;
        HasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }

    BOOL IsInvalid;     //是否有效
    BOOL IsDirectory;   // 是否是目录 0否 1是
    BOOL HasNext;       // 是否有下一个文件 0否 1是
    char szFileName[256];   // 文件名

}FILEINFO, *PFILEINFO;

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
    CPackage pack(1, (BYTE*)result.c_str(), result.size()); //打包数据的构造函数重载
    Dump((BYTE*)pack.Data(), pack.Size());
    //CServerSocket::getInstance()->Send(pack);

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
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        //listFileInfos.push_back(finfo);  // 不用列表的形式，而是读到一个文件就发一个文件
        // 随读 随发
        CPackage pack(2, (BYTE*) & finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限，访问目录！"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if (hfind = (_findfirst("*", &fdata)) == -1)
    {
        OutputDebugString(_T("没有找到任何文件！"));
        return -3;
    }
    // 上面是各种文件查找出错的情况
    // 下面是循环查找文件夹目录下的内容
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //listFileInfos.push_back(finfo);
        CPackage pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));

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
            //int count = 0;
            //CServerSocket* pserver = CServerSocket::getInstance();
            //if (pserver->InitSocket() == false)
            //{
            //    MessageBox(NULL, _T("网络初始化异常，未成功初始化，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //while(CServerSocket::getInstance() != NULL)
            //{
            //    if (pserver->AcceptClient() == false)
            //    {
            //        if (count >= 3)
            //        {
            //            MessageBox(NULL, _T("多次无法正常接入用户，直接退出"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    int ret = pserver->DealCommand();
            //    // TODO
            //}

            int cmd = 1;
            switch (cmd)
            {
            case 1: // 查看磁盘分区
                MakeDriverInfo();
                break;
            case 2: // 查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            case 3: //打开文件
                RunFile();
                break;
            case 4:
                DownloadFile();
                break;
            default:
                break;
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
