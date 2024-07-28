// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialogEx)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialogEx)
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{// 960 * 540

	CRect clientRect;
	if(isScreen)
		ScreenToClient(&point); // 全局坐标转换到客户端区域坐标

	// 本地坐标转换到远程坐标
	m_picture.GetWindowRect(clientRect);
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	int width = 3840;
	int height = 2160;
	int x = point.x * width / width0;
	int y = point.y * height / height0;

	return CPoint(x, y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0)
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull())	// 缓冲内有数据
		{
			// 读取缓冲内的数据
			CRect rect;
			m_picture.GetWindowRect(rect);
			// 窗口缩放
			pParent->GetImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->GetImage().Destroy();
			pParent->SetImageStatus();	// 设置缓冲区内容为false，代表缓冲区内数据已经读了，然后置空

		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

// 左键按下
void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;	// 左键
	event.nAction = 2;	// 按下
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialogEx::OnLButtonDown(nFlags, point);
}

// 左键双击
void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;	// 左键
	event.nAction = 1;	// 双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialogEx::OnLButtonDblClk(nFlags, point);
}

// 左键弹起
void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;	// 左键
	event.nAction = 3;	// 弹起  
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialogEx::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;	// 右键
	event.nAction = 2;	// 按下
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialogEx::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;	// 右键
	event.nAction = 1;	// 双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialogEx::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;	// 右键	
	event.nAction = 3;	// 弹起
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialogEx::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 8;	// 没有按键
	event.nAction = 0;	// 移动
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialogEx::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point);
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point, true);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;
	event.nAction = 0;	// 单击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);


}
