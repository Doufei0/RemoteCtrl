/*
	�������һ��������
*/

#pragma once
#include "pch.h"
#include "framework.h"

constexpr int BUFFER_SIZE = 4096;

class CPackage
{
public:
	CPackage() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPackage(WORD Cmd, const BYTE* pData, size_t nSize) { // ���ڰ����ݵĴ���ع�
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
	CPackage(const CPackage& p){
		sHead = p.sHead;
		nLength = p.nLength;
		sCmd = p.sCmd;
		strData = p.strData;
		sSum = p.sSum;
	}
	CPackage(const BYTE* pData, size_t& nSize) { // ���ڰ����ݽ�������Ӧ�ĳ�Ա������
		size_t i = 0;
		for (; i < nSize; ++i)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // ʶ�𵽰�ͷ
			{
				sHead = *(WORD*)(pData + i);  // ȡ��ͷ
				i += 2;
				break;
			}
		}
		// �����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
		if (i + 4 + 2 + 2 > nSize) // 4��2��2 �ֱ��� DWORD nLength �� WORD sCmd �� sSum ����ռ�õĳ���
		{
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) // ��δ��ȫ���գ����ؽ���ʧ��
		{
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2); // -2-2 ��ʾ��ȥsCmd��sSum ��ռ���ֽڳ���,ʣ�µľ���strData��ռ�õ��ֽ�
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
		if(this != &p)
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

	int Size() { // �������������ݵĴ�С
		return nLength + 6; // +6 ����Ϊ headռ��2�ֽڣ��Լ�nLengthռ��4�ֽ�
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
	WORD sHead;			 // ��ͷ�̶���־λ 0xFEFF  2�ֽ�
	DWORD nLength;		 // ������ (��������+������+��У��) �ĳ���  4�ֽ�
	WORD sCmd;			 // ��������	2�ֽ�
	std::string strData; // ������
	WORD sSum;			 // ��У��	2�ֽ�
	std::string strOut;  // ������������
};



class CServerSocket
{
private:
	static CServerSocket* m_instance;
	SOCKET m_socket;
	SOCKET m_client;
	CPackage m_package;

	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_client = ss.m_client;
		m_socket = ss.m_socket;
	}

	CServerSocket() {
		m_client = INVALID_SOCKET; // -1
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_socket = socket(PF_INET, SOCK_STREAM, 0);

	}
	~CServerSocket() {
		closesocket(m_socket);
		WSACleanup();
	}

	BOOL InitSockEnv()
	{
		// �׽��ֳ�ʼ��
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) //todo: ����ֵ����
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
		if (m_client == -1)
			return -1;
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;  //��ʼ��һ������ index�����ڸ��ٵ�ǰ�ѽ��������� buffer �е�λ��
		while (1)
		{
			// ���ݴ洢����ʼλ��buffer+index ��ʼ��������󳤶�ΪBUFFER_SIZE-index������
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0); 
			if (len <= 0)
				return -1;
			// �������� index��ʹ��ָ�� buffer ����һ�����õ�λ��
			index += len;
			//  len ����Ϊ��ǰ�ѽ������ݵ��ܳ���
			len = index;
			m_package = CPackage((BYTE*)buffer, len);
			if (len > 0)
			{
				// �� buffer ���ѽ����������ƶ��� buffer �Ŀ�ʼλ�ã��Ա���������µ�����
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				// �������� index�������Ѵ�������ݳ���
				index -= len;
				return m_package.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, int nSize) {
		if (m_client == -1)
			return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPackage& pack) {
		if (m_client == -1)
			return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

};


