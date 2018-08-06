#include "stdafx.h"
#include "WndManager.h"
#include "Plot.h"
#include "gdiPlusUse.h"


extern HINSTANCE hInst;
extern HWND g_hMDIClientWnd;
extern BOOL bLBDown;
extern POINT pt1;
extern POINT pt2;
extern MYRECT g_ClientMyRect;
extern CWndManager g_WndManager;
#define  TOOLS_HIGH 40					//上面子窗口的占空比


BOOL CWndManager::AddChildWnd(int wndID, RECT childRect, wndproc pWndProc)
{
	BOOL bRet = false;
	do
	{
		MDICREATESTRUCT MDIChildCreateStruct;
		MDIChildCreateStruct.szClass = L"MDIChildWndClass";
		MDIChildCreateStruct.szTitle = NULL;
		MDIChildCreateStruct.hOwner = hInst;
		MDIChildCreateStruct.x = childRect.left;
		MDIChildCreateStruct.y = childRect.top;
		MDIChildCreateStruct.cx = childRect.right - MDIChildCreateStruct.x;
		MDIChildCreateStruct.cy = childRect.bottom - MDIChildCreateStruct.y;
		MDIChildCreateStruct.style = WS_CHILD | WS_VISIBLE | WS_BORDER;
		//MDIChildCreateStruct2.style = WS_CHILD | WS_VISIBLE | WS_THICKFRAME;
		MDIChildCreateStruct.lParam = 0;




		HWND hNewChildWnd = (HWND)SendMessage(m_hClientWnd, WM_MDICREATE, 0, (LPARAM)&MDIChildCreateStruct);
		if (hNewChildWnd == NULL)
			break;




		CChildWnd *pChildWndObj = new CChildWnd();
		pChildWndObj->SetWndID(wndID);
		pChildWndObj->SetWnd(hNewChildWnd);

		m_ChildWndMap[pChildWndObj] = pWndProc;	//必须放在下一句之前，不然消息发出去了，但是回调函数还没指定，都不知道调用谁
		SendMessage(hNewChildWnd, WM_CREATE, 0, 0);

		bRet = true;
	} while (0);


	return bRet;
};

LRESULT ToolWndProc(CChildWnd* pChildWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmID, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	HWND hWndChild = pChildWnd->GetWnd();
	switch (message)
	{
	case WM_CREATE:
	{
		CXUIBase* pXUI = new CXUITool();
		pXUI->InitUI(52, 40, hWndChild);
		pXUI->AddItem(E_XUITOOL_START, _T("imag\\start.png"), _T("开始"));
		pXUI->AddItem(E_XUITOOL_PAUSE, _T("imag\\pause.png"), _T("暂停"));
		pXUI->AddItem(E_XUITOOL_ZOOMIN, _T("imag\\zoom_in.png"), _T("放大"));
		pXUI->AddItem(E_XUITOOL_ZOOMOUT, _T("imag\\zoom_out.png"), _T("缩小"));
		pXUI->AddItem(E_XUITOOL_MOVE, _T("imag\\move.png"), _T("移动"));
		pXUI->AddItem(E_XUITOOL_SPLIT, _T("imag\\wndsplit.png"), _T("多窗口"));
		pXUI->AddItem(E_XUITOOL_MERGE, _T("imag\\wndmerge.png"), _T("单窗口"));
		pXUI->AddItem(E_XUITOOL_NETCONN, _T("imag\\netconn.png"), _T("网络传输"));
		pXUI->SetToolTipWnd(g_hMDIClientWnd);
		pChildWnd->OnCreate(NULL, pXUI);
		break;
	}
	case WM_PAINT:
		hdc = BeginPaint(hWndChild, &ps);
		pChildWnd->GetXUI()->OnPaint();
		EndPaint(hWndChild, &ps);
		break;
	case WM_MOUSEMOVE:
	{
		//让系统监视 WM_MOUSELEAVE 的消息
		/*
		* @Para  第一个是结构体的大小；第二个是监视的时间，第三个是窗口指针，第三个是探查的时间间隔
		*/
		TRACKMOUSEEVENT tcMouseEvt = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWndChild, HOVER_DEFAULT };
		TrackMouseEvent(&tcMouseEvt);


		pChildWnd->GetXUI()->OnMouseMove(wParam, lParam);
		break;
	}
	case WM_MOUSELEAVE:
		if (pChildWnd->GetXUI()->GetItemOver() != NULL)
			pChildWnd->GetXUI()->GetItemOver()->SetItemStatus(E_ItemStatus_Normal);
		pChildWnd->GetXUI()->SetItemOver(NULL);
		InvalidateRect(hWndChild, &(pChildWnd->GetXUI()->GetXUIRect()), FALSE);		//让UI rect刷新重绘 发出WM_PAINT消息
		break;
	case WM_LBUTTONDOWN:
	{
		//通知其他子窗口失去焦点
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			if (it->first->GetWnd() != hWndChild)
				SendMessage(it->first->GetWnd(), WM_ACTIVATE, 0, 0);
			++it;
		}

		pChildWnd->GetXUI()->OnLButtonDown(wParam, lParam);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		//通知其他子窗口失去焦点
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			if (it->first->GetWnd() != hWndChild)
				SendMessage(it->first->GetWnd(), WM_ACTIVATE, 0, 0);
			++it;
		}
		break;
	}
	case WM_XUI_MSG:
		pChildWnd->GetXUI()->OnXUIMSG(wParam, lParam);
		break;
	case WM_SIZE:
		MoveWindow(hWndChild, g_ClientMyRect.x, g_ClientMyRect.y, g_ClientMyRect.width, TOOLS_HIGH, true);
		break;
	case WM_DESTROY:
		DestroyWindow((HWND)hWndChild);
		//PostQuitMessage(0);		//不能用该函数，会导致整个程序退出的
		break;
	default:
		//return DefWindowProc(hWndChild, message, wParam, lParam);					//SDI
		return DefMDIChildProc(hWndChild, message, wParam, lParam);					//MDI
	}

	return 0;

};

LRESULT PlotWndProc(CChildWnd* pChildWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmID, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HWND hWndChild = pChildWnd->GetWnd();
	switch (message)
	{	
	case WM_CREATE:
	{
		CXUIBase* pXUI = new CXUIMenu();
		CPlot* pPlot = new CPlot();

		pPlot->Init(hWndChild);						//初始化OpenGL绘图环境

		pXUI->InitUI(200, 30, hWndChild);
		pXUI->AddItem(E_XUIMenu_TimeDomain, NULL, _T("时域"));
		pXUI->AddItem(E_XUIMenu_FreqDomain, NULL, _T("频域"));
		pXUI->AddItem(E_XUIMenu_Constellation, NULL, _T("星座图"));

		pChildWnd->OnCreate(pPlot, pXUI);
		break;
	}
	case WM_SIZE:
	{
		pChildWnd->GetXUI()->SetXUIStatus(E_XUI_Invisible);
		int wndID = pChildWnd->GetWndID();
		switch (wndID)
		{
		case E_WND_Plot_Left:		//绘图的左边窗口
		{
			//通知其他子窗口失去焦点
			BOOL bHide = false;
			childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
			while (it != g_WndManager.GetChildWndMap()->end())
			{
				if (it->first->GetWndStatus() == E_WND_Hide)	//有窗口隐藏，那就只可能是右边画图窗口
				{
					bHide = true;
					break;
				}
				++it;
			}
			if (bHide)
				MoveWindow(hWndChild, g_ClientMyRect.x, g_ClientMyRect.y + TOOLS_HIGH, g_ClientMyRect.width, g_ClientMyRect.heigh - TOOLS_HIGH, true);
			else
				MoveWindow(hWndChild, g_ClientMyRect.x, g_ClientMyRect.y + TOOLS_HIGH, g_ClientMyRect.width / 2, g_ClientMyRect.heigh - TOOLS_HIGH, true);
			pChildWnd->GetPlotObj()->OnReshape();
			break;
		}
		case E_WND_Plot_Right:		//绘图的右边窗口
			if (pChildWnd->GetWndStatus() == E_WND_Hide)
				MoveWindow(hWndChild, 0, 0, 1, 1, true);
			else
				MoveWindow(hWndChild, g_ClientMyRect.x + g_ClientMyRect.width / 2, g_ClientMyRect.y + TOOLS_HIGH, g_ClientMyRect.width / 2, g_ClientMyRect.heigh - TOOLS_HIGH, true);
			pChildWnd->GetPlotObj()->OnReshape();
			break;
		default:
			break;
		}
		break;
	}	
	case WM_LBUTTONDOWN:
	{
		//通知其他子窗口失去焦点
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			if (it->first->GetWnd() !=  hWndChild)
				SendMessage(it->first->GetWnd(), WM_ACTIVATE, 0, 0);
			++it;
		}

		GetCursorPos(&pt1);
		ScreenToClient(hWndChild, &pt1);    // 转屏幕座标,如果用屏幕扩展就会出错

		if (pChildWnd->GetXUI()->GetXUIStatus() == E_XUI_Visible)
		{
			pChildWnd->GetXUI()->SetXUIStatus(E_XUI_Invisible);
			wParam = (WPARAM)&pt1;
			SendMessage(hWndChild, WM_PAINT, wParam, lParam);
			pChildWnd->GetXUI()->OnLButtonDown(wParam, lParam);

			break;
		}

		if (pChildWnd->GetPlotObj()->IsinRect(pt1))
			bLBDown = true;
		else
			bLBDown = false;
		break;
	}

	case WM_LBUTTONUP:
	{
		if (bLBDown)
		{
			GetCursorPos(&pt2);
			ScreenToClient(hWndChild, &pt2);		//如果用屏幕扩展就会出错

			if (pChildWnd->GetPlotObj()->IsinRect(pt1))
			{
				pChildWnd->GetPlotObj()->OnTranslate(pt1, pt2);
				pt1 = pt2;	//很关键，鼠标一直按着不放，左右移动鼠标
			}
		}
		bLBDown = false;	//鼠标左键弹起
		break;
	}
	case WM_RBUTTONDOWN:
	{
		//通知其他子窗口失去焦点
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			if (it->first->GetWnd() != hWndChild)
				SendMessage(it->first->GetWnd(), WM_ACTIVATE, 0, 0);
			++it;
		}


		tagPOINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWndChild, &pt);    // 转屏幕座标,如果用屏幕扩展就会出错

		RECT clientRect;
		GetClientRect(hWndChild, &clientRect);
		if (!PtInRect(&clientRect, pt))  //判断该点是否在此矩形区内
		{
			pChildWnd->GetXUI()->SetXUIStatus(E_XUI_Invisible);
			break;
		}

		if (pChildWnd->GetPlotObj()->IsinRect(pt))
		{
			pChildWnd->GetPlotObj()->Set_m_bShowCoord(true);
			pChildWnd->GetPlotObj()->Set_tagPt(pt);
		}
		else
		{
			pChildWnd->GetXUI()->SetXUIStatus(E_XUI_Visible);
			pChildWnd->GetXUI()->SetPos(pt);
		}

		break;
	}
	case WM_RBUTTONUP:
	{
		pChildWnd->GetPlotObj()->Set_m_bShowCoord(false);
		break;
	}
	case WM_MOUSEWHEEL:
		if ((INT)wParam > 0)
			pChildWnd->GetPlotObj()->OnScale(1.25);
		else
			pChildWnd->GetPlotObj()->OnScale(0.8);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWndChild, &ps);
		// TODO:  在此添加任意绘图代码...
		pChildWnd->GetPlotObj()->GetMaxMinValue();
		pChildWnd->GetPlotObj()->Draw();
		if (pChildWnd->GetXUI()->GetXUIStatus() == E_XUI_Visible)
		{
			pChildWnd->GetXUI()->OnPaint();
		}
		EndPaint(hWndChild, &ps);
		break;
	case WM_MOUSEMOVE:
		if (pChildWnd->GetXUI()->GetXUIStatus() == E_XUI_Visible)
		{
			pChildWnd->GetXUI()->OnMouseMove(wParam, lParam);
		}

		break;
	case WM_XUI_MSG:
		pChildWnd->GetXUI()->OnXUIMSG(wParam, lParam);
		break;
	case WM_ACTIVATE:	//窗口失去焦点
		pChildWnd->GetXUI()->SetXUIStatus(E_XUI_Invisible);
		break;

	case WM_DESTROY:

		DestroyWindow((HWND)hWndChild);
		//PostQuitMessage(0);		//不能用该函数，会导致整个程序退出的
		break;
	default:
		//return DefWindowProc(hWndChild, message, wParam, lParam);					//SDI
		return DefMDIChildProc(hWndChild, message, wParam, lParam);				//MDI
	}
	return 0;
}
