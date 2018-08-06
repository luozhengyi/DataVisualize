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
#define  TOOLS_HIGH 40					//�����Ӵ��ڵ�ռ�ձ�


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

		m_ChildWndMap[pChildWndObj] = pWndProc;	//���������һ��֮ǰ����Ȼ��Ϣ����ȥ�ˣ����ǻص�������ûָ��������֪������˭
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
		pXUI->AddItem(E_XUITOOL_START, _T("imag\\start.png"), _T("��ʼ"));
		pXUI->AddItem(E_XUITOOL_PAUSE, _T("imag\\pause.png"), _T("��ͣ"));
		pXUI->AddItem(E_XUITOOL_ZOOMIN, _T("imag\\zoom_in.png"), _T("�Ŵ�"));
		pXUI->AddItem(E_XUITOOL_ZOOMOUT, _T("imag\\zoom_out.png"), _T("��С"));
		pXUI->AddItem(E_XUITOOL_MOVE, _T("imag\\move.png"), _T("�ƶ�"));
		pXUI->AddItem(E_XUITOOL_SPLIT, _T("imag\\wndsplit.png"), _T("�ര��"));
		pXUI->AddItem(E_XUITOOL_MERGE, _T("imag\\wndmerge.png"), _T("������"));
		pXUI->AddItem(E_XUITOOL_NETCONN, _T("imag\\netconn.png"), _T("���紫��"));
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
		//��ϵͳ���� WM_MOUSELEAVE ����Ϣ
		/*
		* @Para  ��һ���ǽṹ��Ĵ�С���ڶ����Ǽ��ӵ�ʱ�䣬�������Ǵ���ָ�룬��������̽���ʱ����
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
		InvalidateRect(hWndChild, &(pChildWnd->GetXUI()->GetXUIRect()), FALSE);		//��UI rectˢ���ػ� ����WM_PAINT��Ϣ
		break;
	case WM_LBUTTONDOWN:
	{
		//֪ͨ�����Ӵ���ʧȥ����
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
		//֪ͨ�����Ӵ���ʧȥ����
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
		//PostQuitMessage(0);		//�����øú������ᵼ�����������˳���
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

		pPlot->Init(hWndChild);						//��ʼ��OpenGL��ͼ����

		pXUI->InitUI(200, 30, hWndChild);
		pXUI->AddItem(E_XUIMenu_TimeDomain, NULL, _T("ʱ��"));
		pXUI->AddItem(E_XUIMenu_FreqDomain, NULL, _T("Ƶ��"));
		pXUI->AddItem(E_XUIMenu_Constellation, NULL, _T("����ͼ"));

		pChildWnd->OnCreate(pPlot, pXUI);
		break;
	}
	case WM_SIZE:
	{
		pChildWnd->GetXUI()->SetXUIStatus(E_XUI_Invisible);
		int wndID = pChildWnd->GetWndID();
		switch (wndID)
		{
		case E_WND_Plot_Left:		//��ͼ����ߴ���
		{
			//֪ͨ�����Ӵ���ʧȥ����
			BOOL bHide = false;
			childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
			while (it != g_WndManager.GetChildWndMap()->end())
			{
				if (it->first->GetWndStatus() == E_WND_Hide)	//�д������أ��Ǿ�ֻ�������ұ߻�ͼ����
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
		case E_WND_Plot_Right:		//��ͼ���ұߴ���
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
		//֪ͨ�����Ӵ���ʧȥ����
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			if (it->first->GetWnd() !=  hWndChild)
				SendMessage(it->first->GetWnd(), WM_ACTIVATE, 0, 0);
			++it;
		}

		GetCursorPos(&pt1);
		ScreenToClient(hWndChild, &pt1);    // ת��Ļ����,�������Ļ��չ�ͻ����

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
			ScreenToClient(hWndChild, &pt2);		//�������Ļ��չ�ͻ����

			if (pChildWnd->GetPlotObj()->IsinRect(pt1))
			{
				pChildWnd->GetPlotObj()->OnTranslate(pt1, pt2);
				pt1 = pt2;	//�ܹؼ������һֱ���Ų��ţ������ƶ����
			}
		}
		bLBDown = false;	//����������
		break;
	}
	case WM_RBUTTONDOWN:
	{
		//֪ͨ�����Ӵ���ʧȥ����
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			if (it->first->GetWnd() != hWndChild)
				SendMessage(it->first->GetWnd(), WM_ACTIVATE, 0, 0);
			++it;
		}


		tagPOINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWndChild, &pt);    // ת��Ļ����,�������Ļ��չ�ͻ����

		RECT clientRect;
		GetClientRect(hWndChild, &clientRect);
		if (!PtInRect(&clientRect, pt))  //�жϸõ��Ƿ��ڴ˾�������
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
		// TODO:  �ڴ���������ͼ����...
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
	case WM_ACTIVATE:	//����ʧȥ����
		pChildWnd->GetXUI()->SetXUIStatus(E_XUI_Invisible);
		break;

	case WM_DESTROY:

		DestroyWindow((HWND)hWndChild);
		//PostQuitMessage(0);		//�����øú������ᵼ�����������˳���
		break;
	default:
		//return DefWindowProc(hWndChild, message, wParam, lParam);					//SDI
		return DefMDIChildProc(hWndChild, message, wParam, lParam);				//MDI
	}
	return 0;
}
