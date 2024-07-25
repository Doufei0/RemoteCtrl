/*
	单例设计一个客户端
*/

#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>

constexpr int BUFFER_SIZE = 4096;

class CPackage
{
public:
	CPackage() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPackage(WORD Cmd, const BYTE* pData, size_t nSize) { // 用于包数据的打包重构
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = Cmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (size_t i = 0; i < strData.size(); ++i)
		{
			sSum += BYTE(strData[i]) & 0xFF;
		}
	}
	CPackage(const CPackage& p) {
		sHead = p.sHead;
		nLength = p.nLength;
		sCmd = p.sCmd;
		strData = p.strData;
		sSum = p.sSum;
	}
	CPackage(const BYTE* pData, size_t& nSize) { // 用于包数据解析到对应的成员变量中
		size_t i = 0;
		for (; i < nSize; ++i)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // 识别到包头
			{
				sHead = *(WORD*)(pData + i);  // 取包头
				i += 2;
				break;
			}
		}
		// 包数据可能不全，或者包头未能全部接收到
		if (i + 4 + 2 + 2 > nSize) // 4，2，2 分别是 DWORD nLength 和 WORD sCmd 和 sSum 变量占用的长度
		{
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) // 包未完全接收，返回解析失败
		{
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2); // -2-2 表示减去sCmd和sSum 所占的字节长度,剩下的就是strData所占用的字节
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); ++j)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = i;
			return;
		}
		nSize = 0;

	}
	CPackage& operator=(const CPackage& p) {
		if (this != &p)
		{
			sHead = p.sHead;
			nLength = p.nLength;
			sCmd = p.sCmd;
			strData = p.strData;
			sSum = p.sSum;
		}
		return *this;
	}
	~CPackage() {}

	int Size() { // 返回整个包数据的大小
		return nLength + 6; // +6 是因为 head占用2字节，自己nLength占用4字节
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();

		*(WORD*)pData = sHead;
		pData += 2;

		*(DWORD*)pData = nLength;
		pData += 4;

		*(WORD*)pData = sCmd;
		pData += 2;

		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();

		*(WORD*)pData = sSum;
		return strOut.c_str();
	}


public:
	WORD sHead;			 // 包头固定标志位 0xFEFF  2字节
	DWORD nLength;		 // 包长度 (控制命令+包数据+和校验) 的长度  4字节
	WORD sCmd;			 // 控制命令	2字节
	std::string strData; // 包数据
	WORD sSum;			 // 和校验	2字节
	std::string strOut;  // 整个包的数据
};

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction; // 点击、移动、双击
	WORD nButton; // 左键、右键、中键
	POINT ptXY;	  // 坐标

}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}

	BOOL IsInvalid;     //是否有效
	BOOL IsDirectory;   // 是否是目录 0否 1是
	BOOL HasNext;       // 是否有下一个文件 0否 1是
	char szFileName[256];   // 文件名

}FILEINFO, * PFILEINFO;

// 给出声明
std::string GetErrInfo(int wsaErrCode); 

class CClientSocket
{
private:
	static CClientSocket* m_instance;
	SOCKET m_socket;
	CPackage m_package;
	std::vector<char> m_buffer;

	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {
		m_socket = ss.m_socket;
	}

	CClientSocket() {
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE); // 设置缓冲区大小
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	~CClientSocket() {
		closesocket(m_socket);
		WSACleanup();
	}

	BOOL InitSockEnv()
	{
		// 套接字初始化
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) //todo: 返回值处理
			return FALSE;

		return TRUE;
	}

	static void releaseInstance() {
		if (m_instance != NULL)
		{
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;


public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL)
			m_instance = new CClientSocket();
		return m_instance;
	}

	bool InitSocket(int nIP, int nPort) {
		if (m_socket != INVALID_SOCKET)
			CloseClient();
		m_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (m_socket == -1)
			return false;
		TRACE("m_socket = %d\r\n", m_socket);

		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(nIP);
		//serv_addr.sin_addr.s_addr = inet_addr(strIPAddress.c_str());
		serv_addr.sin_port = htons(nPort);
		//TRACE("%d  ----- none: %d", (unsigned)(serv_addr.sin_addr.s_addr), (unsigned)INADDR_NONE);
		// 遇到的问题，会进入这个判断语句认为IP不存在
		/*if (serv_addr.sin_addr.s_addr == -5);
		{
			AfxMessageBox("指定的IP地址不存在 !");
			return false;
		}*/
		// connect
		int ret = connect(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr));
		
		if (ret == -1)
		{
			AfxMessageBox("Connect Error !");
			TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}


	int DealCommand() {
		if (m_socket == -1)
			return -1;
		//char* buffer = new char[BUFFER_SIZE];
		char* buffer = m_buffer.data();
		static size_t index = 0;
		while (1)
		{
			size_t len = recv(m_socket, buffer + index, BUFFER_SIZE - index, 0);
			if ((len <= 0) && (index <= 0))
				return -1;
			index += len;
			len = index;
			m_package = CPackage((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_package.sCmd;
			}
		}
		return -1;

		//memset(buffer, 0, BUFFER_SIZE);
		//size_t index = 0;  //初始化一个索引 index，用于跟踪当前已接收数据在 buffer 中的位置
		//while (1)
		//{
		//	// 数据存储的起始位置buffer+index 开始，接收最大长度为BUFFER_SIZE-index的数据
		//	size_t len = recv(m_socket, buffer + index, BUFFER_SIZE - index, 0);
		//	if (len <= 0)
		//		return -1;
		//	// 更新索引 index，使其指向 buffer 中下一个可用的位置
		//	index += len;
		//	//  len 更新为当前已接收数据的总长度
		//	len = index;
		//	m_package = CPackage((BYTE*)buffer, len);
		//	if (len > 0)
		//	{
		//		// 将 buffer 中已解析的数据移动到 buffer 的开始位置，以便后续接收新的数据
		//		memmove(buffer, buffer + len, BUFFER_SIZE - len);
		//		// 更新索引 index，减少已处理的数据长度
		//		index -= len;
		//		return m_package.sCmd;
		//	}
		//}
		//return -1;
	}

	bool Send(const char* pData, int nSize) {
		TRACE("m_socket = %d\r\n", m_socket);

		if (m_socket == -1)
			return false;
		return send(m_socket, pData, nSize, 0) > 0;
	}

	bool Send(CPackage& pack) {
		if (m_socket == -1)
			return false;
		return send(m_socket, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) {
		if ((m_package.sCmd == 2) || (m_package.sCmd == 3)
			|| (m_package.sCmd == 4))
		{
			strPath = m_package.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_package.sCmd == 5)
		{
			memcpy(&mouse, m_package.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	CPackage& GetPackage() {
		return m_package;
	}

	void CloseClient() {
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}

};


