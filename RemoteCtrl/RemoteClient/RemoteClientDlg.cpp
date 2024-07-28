
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

int CRemoteClientDlg::SendCommandPackage(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCSTR)m_nPort));
	if (ret == false)
	{
		AfxMessageBox("网络初始化失败！");
		return -1;
	}
	CPackage pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Send ret = %d\r\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("ACK: %d\r\n", cmd);
	if(bAutoClose)
		pClient->CloseClient();
	return cmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_test, &CRemoteClientDlg::OnBnClickedBtntest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_WATCH, &CRemoteClientDlg::OnBnClickedBtnWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7F000001;
	m_nPort = _T("9999");
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;	// 初始化缓存中没有数据
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedBtntest()
{
	SendCommandPackage(1999);

}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPackage(1); // 发送命令1，代表查看磁盘分区MakeDriverInfo()
	if (ret == -1)
	{
		AfxMessageBox("命令处理失败！");
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPackage().strData;
	std::string tmp;
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); ++i)
	{
		if (drivers[i] == ',')
		{
			tmp += ":";
			HTREEITEM hTemp = m_Tree.InsertItem(tmp.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			tmp.clear();
			continue;
		}
		tmp += drivers[i];
	}


}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();

	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	Sleep(50);
	CClientSocket* pClient = NULL;
	do
	{
		pClient = CClientSocket::getInstance();
	} while (pClient == NULL);
	ULONGLONG tick = GetTickCount64();
	while (true)
	{
		if (m_isFull == false)	// 缓存中没有数据
		{
			// 准备指令6，监控屏幕截图并发送
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			if (ret == 6)	// 发送成功
			{
				// 接收数据到缓存中
				BYTE* pData = (BYTE*)pClient->GetPackage().strData.c_str();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMem == NULL)
				{
					TRACE("内存不足！");
					Sleep(1);
					continue;
				}
				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
				if (hRet == S_OK)
				{
					ULONG length = 0;
					pStream->Write(pData, pClient->GetPackage().strData.size(), &length);
					LARGE_INTEGER bg = { 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);
					m_image.Load(pStream);
					m_isFull = true;
				}
			}
			else { 
				Sleep(1);
			}
		}
		else { 
			Sleep(1);
		}
	}
}

void CRemoteClientDlg::threadEntryDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadEntryDownFile();

	_endthread();
}

void CRemoteClientDlg::threadEntryDownFile()
{
	// 选中右边列表中的文件
	int nListSelected = m_List.GetSelectionMark();
	// 获取文件的文件名
	CString strFileName = m_List.GetItemText(nListSelected, 0);
	// 弹出一个对话框，选择下载指定文件到本地的哪个位置
	CFileDialog dlg(FALSE, NULL,
		strFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, this);
	// 模态的，当前窗口不结束，无法点击后面的窗口
	if (dlg.DoModal() == IDOK)
	{
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL)
		{
			AfxMessageBox("本地无权限保存该文件，或文件无法创建！");
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		// 获取当前文件所在的左边磁盘
		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		// 生成当前文件的路径
		strFileName = GetPath(hSelected) + strFileName;
		TRACE("%s\r\n", LPCSTR(strFileName));
		//int ret = SendCommandPackage(4, false, (BYTE*)(LPCSTR)strFileName, strFileName.GetLength()); // 新开的线程的消息循环机制
		int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFileName);
		if (ret < 0)
		{
			AfxMessageBox("执行下载失败！");
			TRACE("执行下载失败, ret = %d\r\n", ret);
			return;
		}
		CClientSocket* pClient = CClientSocket::getInstance();
		long long nLength = *(long long*)CClientSocket::getInstance()->GetPackage().strData.c_str();
		if (nLength == 0)
		{
			AfxMessageBox("文件长度为零或无法读取文件！");
			return;
		}
		long long nCount = 0;
		while (nCount < nLength)
		{
			ret = pClient->DealCommand();
			if (ret < 0)
			{
				AfxMessageBox("传输失败！");
				TRACE("传输失败, ret = %d\r\n", ret);
				break;
			}
			fwrite(pClient->GetPackage().strData.c_str(), 1, pClient->GetPackage().strData.size(), pFile);
			nCount += pClient->GetPackage().strData.size();
		}
		fclose(pFile);
		pClient->CloseClient();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成！"), _T("完成"));
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do
	{
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do
	{
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)
			m_Tree.DeleteItem(hSub);

	} while (hSub != NULL);
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	// 点击空的位置
	if (hTreeSelected == NULL)
		return;
	// 如果双击的是文件，直接返回
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)
		return;
	// 如果双击文件夹，则继续进入文件夹中
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPackage(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPackage().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext)
	{
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) // 如果是文件夹目录
		{
			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..")
			{
				int cmd = pClient->DealCommand();
				TRACE("ACK :%d\r\n", cmd);
				if (cmd < 0)
					break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPackage().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ACK :%d\r\n", cmd);
		if (cmd < 0)
			break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPackage().strData.c_str();

	}

	pClient->CloseClient();
}

void CRemoteClientDlg::FileRefresh()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = SendCommandPackage(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPackage().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext)
	{
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) // 不是文件夹目录
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ACK :%d\r\n", cmd);
		if (cmd < 0)
			break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPackage().strData.c_str();
	}
	pClient->CloseClient();
}


// 双击文件夹显示文件夹里内容
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}

// 单击文件夹显示文件夹里内容
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	// 加载文件夹中的内容
	LoadFileInfo();
}

// 右键某个文件执行对应功能
void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	// 获取鼠标位置
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	// 鼠标右键的内容
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0)	// 如果右键点击的位置为空
		return;
	// 单击到LIST中的某个文件上
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

// 右键单击下载文件，弹出文件对话框下载到指定目录
void CRemoteClientDlg::OnDownloadFile()
{
	// 下载大文件时候会阻塞，那么如何避免下载时程序阻塞不能操作？ 
	// 新开一个线程

	// ---- 添加线程函数 ----
	_beginthread(CRemoteClientDlg::threadEntryDownFile, 0, this);
	BeginWaitCursor();
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
}

// 右键单击删除文件
void CRemoteClientDlg::OnDeleteFile()
{
	// 删除文件
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFileName = m_List.GetItemText(nSelected, 0);
	// 文件的路径
	strFileName = strPath + strFileName;
	int ret = SendCommandPackage(9, true, (BYTE*)(LPCSTR)strFileName, strFileName.GetLength());
	if (ret < 0)
	{
		AfxMessageBox("删除文件执行失败！");
		return;
	}
	// 刷新界面 : 删除之后刷新文件界面，把刚才删除的不显示
	FileRefresh();
}

// 右键单击打开文件
void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFileName = m_List.GetItemText(nSelected, 0);
	// 文件的路径
	strFileName = strPath + strFileName;
	int ret = SendCommandPackage(3, true, (BYTE*)(LPCSTR)strFileName, strFileName.GetLength());
	if (ret < 0)
	{
		AfxMessageBox("打开文件执行失败！");
		return;
	}

}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd)
	{
	case 4:
		{
			CString strFileName = (LPCSTR)lParam;
			ret = SendCommandPackage(cmd, wParam & 1, (BYTE*)(LPCSTR)strFileName, strFileName.GetLength());
		}
		break;
	case 6: 
		{
			ret = SendCommandPackage(cmd, wParam & 1);
		}
		break;
	default:
		ret = -1;
	}
	return ret;
}



void CRemoteClientDlg::OnBnClickedBtnWatch()
{
	CWatchDialog dlg(this);

	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();

}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}
