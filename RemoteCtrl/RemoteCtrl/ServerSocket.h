/*
	单例设计一个服务器
*/

#pragma once
#include "pch.h"
#include "framework.h"


class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL)
			m_instance = new CServerSocket();
		return m_instance;
	}

	bool InitSocket() {
		if (m_socket == -1)
			return false;

		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(9999);

		// bind 
		if (bind(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
			return false;

		//listen
		if (listen(m_socket, 1) == -1)
			return false;

		return true;
	}

	bool AcceptClient() {
		sockaddr_in clnt_addr;
		int clnt_sz = sizeof(clnt_addr);
		m_client = accept(m_socket, (sockaddr*)&clnt_addr, &clnt_sz);
		if (m_client == -1)
		{
			return false;
		}

		return true;

	}

	int DealCommand() {
		char buffer[1024] = "";
		while (1)
		{
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0)
				return -1;

			// TODO:处理
		}
	}

	bool Send(const char* pData, int nSize) {
		if (m_client == -1)
			return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

private:
	static CServerSocket* m_instance;
	SOCKET m_socket;
	SOCKET m_client;

	CServerSocket& operator=(const CServerSocket& ss){}
	CServerSocket(const CServerSocket& ss){
		m_client = ss.m_client;
		m_socket = ss.m_socket;
	}

	CServerSocket() {
		m_client = INVALID_SOCKET; // -1
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_socket = socket(PF_INET, SOCK_STREAM, 0);

	}
	~CServerSocket(){
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
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};


