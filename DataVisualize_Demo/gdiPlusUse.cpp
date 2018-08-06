#include "stdafx.h"
#include "gdiPlusUse.h"
#include "Plot.h"
#include "WndManager.h"

extern CWndManager g_WndManager;


/************************************************************************/
void CXUIToolItem::Draw(HDC hdc)
{
	Gdiplus::Graphics grx(hdc);
	int iItemWidth = GetItemRECT().right - GetItemRECT().left;
	int iItemHeight = GetItemRECT().bottom - GetItemRECT().top;

	//1.���Ʊ���(��䱳��)
	//GDI+�Ĺ���ˢ�ӣ����ˢ�ӱȽ�ǿ�󣬿��Խ��䣬��������ͼƬ��ˢ��;Color�еĵ�һ��������ʾ͸����,255��ʾ��͸��
	Gdiplus::SolidBrush sbrsh(Gdiplus::Color(200, 255,255, 255));
	
	if (GetItemStatus() == E_ItemStatus_Over)	//����Ƶ���UI Item��
	{
		sbrsh.SetColor(Gdiplus::Color(200, 0, 0, 255));
		//POINT pt;
		//GetCursorPos(&pt);
		//ScreenToClient()
		//TOOLTIPS_CLASSW sb;
		//TOOLTIPTEXT bs;
		//MessageBoxExW(NULL, _T("hello"), NULL, NULL, NULL);
		
	}
	grx.FillRectangle(&sbrsh, Gdiplus::Rect(GetItemRECT().left, GetItemRECT().top, iItemWidth, iItemHeight));

	//2.���� Item
	Gdiplus::Image img((GetItemRSCPath())->c_str());
	Gdiplus::Rect itemRECT;
	//��ͼ�괹ֱ����
	itemRECT.X = GetItemRECT().left+6;
	itemRECT.Y = GetItemRECT().top;
	itemRECT.Width = GetItemRECT().right - GetItemRECT().left-12;
	itemRECT.Height = GetItemRECT().bottom - GetItemRECT().top;

	//3.���Ʒָ���
	Gdiplus::Pen newpen(Gdiplus::Color(255, 255, 255), 1);
	grx.DrawLine(&newpen, GetItemRECT().right, GetItemRECT().top, GetItemRECT().right, GetItemRECT().bottom);


	Gdiplus::Status sta = grx.DrawImage(&img, itemRECT);
}

/************************************************************************/

void CXUITool::AddItem(int iItemID, const wchar_t* wszItemRSCPath, const wchar_t* wszCaption)
{
	CXUIItemBase *pUIItem = new CXUIToolItem();
	pUIItem->SetItemID(iItemID);				//Item ID
	pUIItem->SetItemRSCPath(wszItemRSCPath);	//ͼ��·��
	pUIItem->SetItemCaption(wszCaption);		//��ʾ��

	//����UI���ڴ����е�����:��������
	RECT itemRECT;
	itemRECT.top = 0;
	itemRECT.left = GetItemMapSize()* GetItemWidth();
	itemRECT.right = (GetItemMapSize() + 1) * GetItemWidth();
	itemRECT.bottom = itemRECT.top + GetItemHeigh();
	pUIItem->SetItemRECT(itemRECT);
	pUIItem->SetItemStatus(E_ItemStatus_Normal);

	InsertItemMap(iItemID, pUIItem);
}

void CXUITool::OnPaint()
{
	//��ģ���п����Ĵ���
	PAINTSTRUCT ps;
	HDC hdc = GetDC(GetHWD());

	//����˵��ڴ����е�λ��
	//GetClientRect(m_hWnd, &m_rcMenu);
	RECT XUIRect;
	XUIRect.top = 0;
	XUIRect.left = 0;
	XUIRect.right = GetItemWidth() * GetItemMapSize();
	XUIRect.bottom = GetItemHeigh();
	//��ʵ���� rect.left = rect.top = 0;
	int XUIWidth = XUIRect.right - XUIRect.left;
	int XUIHeight = XUIRect.bottom - XUIRect.top;
	//��¼UI���ڵ�rect
	SetXUIRect(XUIRect);

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP hBmp = CreateCompatibleBitmap(hdc, XUIWidth, XUIHeight);
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(memDC, hBmp);
	//���ƴ���


	
	
	//2.���Ʋ˵���
	Gdiplus::Graphics grx(memDC);
	ItemMap::iterator it = GetItemMap().begin();
	while (it != GetItemMap().end())
	{
		(*it).second->Draw(memDC);	//it->first��ʾ����it->second��ʾֵ
		++it;
	}
	
	//������ʾ�� tooltip
	if (GetItemOver() != NULL)
	{
		MoveWindow(GetToolTipWnd(), 0, 0, 0, 0, true);

		RECT itemRect = GetItemOver()->GetItemRECT();
		wstring* wstrCaption = GetItemOver()->GetItemCaption();
		int iLen = wcslen(GetItemOver()->GetItemCaption()->c_str());
		SetWindowTextW(GetToolTipWnd(), GetItemOver()->GetItemCaption()->c_str());
		MoveWindow(GetToolTipWnd(), itemRect.left+5, itemRect.bottom + 15, iLen * 20, 22, true);
	}
	else
	{
		MoveWindow(GetToolTipWnd(), 0, 0, 0, 0, true);
	}
		

	//�����ڴ�DC������DC��
	::BitBlt(hdc, XUIRect.left, XUIRect.top, XUIWidth, XUIHeight, memDC, 0, 0, SRCCOPY);
	::SelectObject(hdc, hOldBmp);
	::DeleteObject(hBmp);
	::ReleaseDC(GetHWD(), memDC);
}

void CXUITool::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(GetHWD(), &pt);

	//�жϹ���ڲ���UI��
	if (!PtInRect(&(GetXUIRect()), pt))
	{
		if (GetItemOver() != NULL)
			GetItemOver()->SetItemStatus(E_ItemStatus_Normal);
		SetItemOver(NULL);
		InvalidateRect(GetHWD(), &(GetXUIRect()), FALSE);		//��UI rectˢ���ػ� ����WM_PAINT��Ϣ
		return;
	}
		

		
		
	//�жϹ�����Ǹ�UI����
	CXUIItemBase* pItem = FindItem(pt);
	if (pItem && pItem !=GetItemOver())		//����ڱ����Ǹ����Ĳ˵�����
	{
		pItem->SetItemStatus(E_ItemStatus_Over);
		if (GetItemOver() != NULL)
			GetItemOver()->SetItemStatus(E_ItemStatus_Normal);
		SetItemOver(pItem);
		InvalidateRect(GetHWD(), &(GetXUIRect()), FALSE);		//��UI rectˢ���ػ� ����WM_PAINT��Ϣ
	}

}

void CXUITool::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	//��ȡ�������
	POINT pt;
	::GetCursorPos(&pt);
	::ScreenToClient(GetHWD(), &pt);

	//��ȡ���ڿͻ�������
	//RECT rectClient;
	//::GetClientRect(GetHWD(), &rectClient);

	//�ж���������ڲ���UI����
	if (PtInRect(&(GetXUIRect()), pt))
	{
		CXUIItemBase* pItem = FindItem(pt);
		if (pItem)
			::PostMessage(GetHWD(), WM_XUI_MSG,
			(WPARAM)pItem->GetItemID(), NULL);
	}

}

void CXUITool::OnXUIMSG(WPARAM wParam, LPARAM LPARAM)
{
	switch (wParam)
	{
	case E_XUITOOL_START:
		CPlot::SetDrawStatus(E_Draw_Fresh);
		break;
	case E_XUITOOL_PAUSE:
		CPlot::SetDrawStatus(E_Draw_Static);
		break;
	case E_XUITOOL_ZOOMIN:
		break;
	case E_XUITOOL_ZOOMOUT:
		break;
	case E_XUITOOL_MOVE:
		break;
	case E_XUITOOL_SPLIT:
	{
		//���������Ӵ��ڣ��ҵ�XUI�������Ӵ���
		HWND hWnd = NULL;
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWndID() == E_WND_Plot_Right)		//���ұߴ������أ���ʵ������ߴ����ڸ���
			{
				it->first->SetWndStatus(E_WND_Show);
				SendMessage(it->first->GetWnd(), WM_SIZE, 0, 0);
			}
			if (it->first->GetWndID() == E_WND_Plot_Left)
				hWnd = it->first->GetWnd();

		}
		SendMessage(hWnd, WM_SIZE, 0, 0);	//���ں����ֹ�ұߴ��ڵ�״̬��û�ı�
		break;
	}

		break;
	case E_XUITOOL_MERGE:
	{
		//���������Ӵ��ڣ��ҵ�XUI�������Ӵ���
		HWND hWnd = NULL;
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWndID() == E_WND_Plot_Right)		//���ұߴ������أ���ʵ������ߴ����ڸ���
			{
				it->first->SetWndStatus(E_WND_Hide);
				SendMessage(it->first->GetWnd(), WM_SIZE, 0, 0);
			}
			if (it->first->GetWndID() == E_WND_Plot_Left) //���ں����ֹ�ұߴ��ڵ�״̬��û�ı�
				hWnd = it->first->GetWnd();
			
		}
		SendMessage(hWnd, WM_SIZE, 0, 0);
		break;
	}
	case E_XUITOOL_NETCONN:
	{
		CPlot::SetDataSource(en_From_Sock);
		CPlot::SetDrawStatus(E_Draw_Fresh);
		break;
	}
	default:
		break;
	}
}



/************************************************************************/
void CXUIMenuItem::Draw(HDC hdc)
{
	Gdiplus::Graphics grx(hdc);
	int iItemWidth = GetItemRECT().right - GetItemRECT().left;
	int iItemHeight = GetItemRECT().bottom - GetItemRECT().top;

	//1.���Ʊ���(��䱳��)
	//GDI+�Ĺ���ˢ�ӣ����ˢ�ӱȽ�ǿ�󣬿��Խ��䣬��������ͼƬ��ˢ��;Color�еĵ�һ��������ʾ͸����,255��ʾ��͸��
	Gdiplus::SolidBrush sbrsh(Gdiplus::Color(255, 230, 230, 230));	//��ɫ

	if (GetItemStatus() == E_ItemStatus_Over)	//����Ƶ���UI Item��
	{
		sbrsh.SetColor(Gdiplus::Color(255, 192, 192, 192));				//��ɫ
	}
	grx.FillRectangle(&sbrsh, Gdiplus::Rect(GetItemRECT().left, GetItemRECT().top, iItemWidth, iItemHeight));
	//grx.FillRectangle(&sbrsh, Gdiplus::Rect(0, 0, iItemWidth, iItemHeight));

	//2.���� Item �˵���
	Gdiplus::RectF textRECT;
	//����������룬����
#define ICON_WIDTH 32
	textRECT.X = GetItemRECT().left + ICON_WIDTH;
	textRECT.Y = GetItemRECT().top;
	textRECT.Width = GetItemRECT().right - textRECT.X;
	textRECT.Height = GetItemRECT().bottom - GetItemRECT().top;


	//��������
	Gdiplus::FontFamily fontFamily(_T("����"));		//��������
	Gdiplus::Font font(&fontFamily, 12);			//���������С
	Gdiplus::StringFormat strFormat;				//�������ֶ�����
	strFormat.SetAlignment(Gdiplus::StringAlignmentNear);		//�������
	strFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);	//�������

	//�û�ˢ�����ֻ���;��ɫ
	Gdiplus::SolidBrush fontBrush(Gdiplus::Color(0, 0, 0));
	grx.DrawString(GetItemCaption()->c_str(), wcslen(GetItemCaption()->c_str()), &font, textRECT, &strFormat, &fontBrush);


	//3.���Ʒָ���,
	Gdiplus::Pen newpen(Gdiplus::Color(255, 255, 255), 1);
	Gdiplus::Status sta = grx.DrawLine(&newpen, GetItemRECT().left + ICON_WIDTH, GetItemRECT().top, GetItemRECT().left + ICON_WIDTH, GetItemRECT().bottom);
	grx.DrawLine(&newpen, GetItemRECT().left + ICON_WIDTH, GetItemRECT().top,
						  GetItemRECT().right,             GetItemRECT().top);
	grx.DrawLine(&newpen, GetItemRECT().left + ICON_WIDTH, GetItemRECT().bottom,
						  GetItemRECT().right            , GetItemRECT().bottom);

	//1.���Ʊ߿�
#define FRAMEWIDTH 1
	newpen.SetColor(Gdiplus::Color(128, 128, 128));
	grx.DrawLine(&newpen, FRAMEWIDTH, GetItemRECT().top,
						  FRAMEWIDTH, GetItemRECT().bottom);		//��߿�
	grx.DrawLine(&newpen, iItemWidth - 2 * FRAMEWIDTH, GetItemRECT().top,
						  iItemWidth - 2 * FRAMEWIDTH, GetItemRECT().bottom);							//�ұ߿�




}

/************************************************************************/
void CXUIMenu::AddItem(int iItemID, const wchar_t* wszItemRSCPath, const wchar_t* wszCaption)
{
	CXUIItemBase *pUIItem = new CXUIMenuItem();
	pUIItem->SetItemID(iItemID);
	pUIItem->SetItemCaption(wszCaption);

	//����˵������ڴ�memDC�е�λ�ã��������У��������������DC�ϵ�
	//UI item ��RECT������ڴ�DC�ģ�UI��RECT���������DC��
	RECT itemRECT;
	itemRECT.top =GetItemMapSize()*GetItemHeigh();
	itemRECT.left = 0;
	itemRECT.right = GetItemWidth();
	itemRECT.bottom = itemRECT.top + GetItemHeigh();
	pUIItem->SetItemRECT(itemRECT);
	pUIItem->SetItemStatus(E_ItemStatus_Normal);

	InsertItemMap(iItemID, pUIItem);
	
}

void CXUIMenu::OnPaint()
{

	//��ģ���п����Ĵ���
	PAINTSTRUCT ps;
	HDC hdc = GetDC(GetHWD());

	//����˵��ڴ����е�λ��,��������
	//GetClientRect(m_hWnd, &m_rcMenu);
	RECT XUIRect;
	XUIRect.top = GetPos().y;
	XUIRect.left = GetPos().x;
	XUIRect.right = XUIRect.left + GetItemWidth();
	XUIRect.bottom = XUIRect.top + GetItemHeigh()* GetItemMapSize();
	//��ʵ���� rect.left = rect.top = 0;
	int XUIWidth = XUIRect.right - XUIRect.left;
	int XUIHeight = XUIRect.bottom - XUIRect.top;
	//��¼UI���ڵ�rect
	SetXUIRect(XUIRect);

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP hBmp = CreateCompatibleBitmap(hdc, XUIWidth, XUIHeight);
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(memDC, hBmp);
	//���ƴ���
	Gdiplus::Graphics grx(memDC);


	//2.���Ʋ˵���
	
	ItemMap::iterator it = GetItemMap().begin();
	while (it != GetItemMap().end())
	{
		(*it).second->Draw(memDC);	//it->first��ʾ����it->second��ʾֵ
		++it;
	}

	//�����ڴ�DC������DC��
	::BitBlt(hdc, XUIRect.left, XUIRect.top, XUIWidth, XUIHeight, memDC, 0, 0, SRCCOPY);
	::SelectObject(hdc, hOldBmp);
	::DeleteObject(hBmp);
	::ReleaseDC(GetHWD(), memDC);

}

void CXUIMenu::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(GetHWD(), &pt);

	RECT temp = GetXUIRect();

	//�жϹ���ڲ���UI��
	if (!PtInRect(&(GetXUIRect()), pt))
		return;

	//�жϹ�����Ǹ�UI����
	//��pt������DC�ϵ�λ��ת��Ϊ�ڴ�DC�ϵ�λ��
	pt.x -= GetXUIRect().left;
	pt.y -= GetXUIRect().top;
	CXUIItemBase* pItem = FindItem(pt);
	if (pItem && pItem != GetItemOver())		//����ڱ����Ǹ����Ĳ˵�����
	{
		pItem->SetItemStatus(E_ItemStatus_Over);
		if (GetItemOver() != NULL)
			GetItemOver()->SetItemStatus(E_ItemStatus_Normal);
		SetItemOver(pItem);
		//InvalidateRect(GetHWD(), &(GetXUIRect()), FALSE);		//��UI rectˢ���ػ�
	}

}

void CXUIMenu::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	//��ȡ�������
	POINT pt;
	//::GetCursorPos(&pt);
	//::ScreenToClient(GetHWD(), &pt);
	pt = *(POINT*)wParam;

	//��ȡ���ڿͻ�������
	//RECT rectClient;
	//::GetClientRect(GetHWD(), &rectClient);

	

	//�ж���������ڲ���UI����
	if (PtInRect(&(GetXUIRect()),pt))
	{
		//��pt������DC�ϵ�λ��ת��Ϊ�ڴ�DC�ϵ�λ��
		pt.x -= GetXUIRect().left;
		pt.y -= GetXUIRect().top;

		CXUIItemBase* pItem = FindItem(pt);
		if (pItem)
			::PostMessage(GetHWD(), WM_XUI_MSG,
			(WPARAM)pItem->GetItemID(), NULL);
	}

}

void CXUIMenu::OnXUIMSG(WPARAM wParam, LPARAM LPARAM)
{
	switch (wParam)
	{
	case E_XUIMenu_TimeDomain:		//����ʱ��
	{
		//���������Ӵ��ڣ��ҵ�XUI�������Ӵ���
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWnd() == GetHWD())
				it->first->GetPlotObj()->SetDrawType(en_Draw_Orignal);
		}
		break;
	}
	case E_XUIMenu_FreqDomain:		//����Ƶ��
	{
		//���������Ӵ����ҵ������˵�������
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWnd() == GetHWD())
				it->first->GetPlotObj()->SetDrawType(en_Draw_Spectrum);

		}
		break;
	}
	case E_XUIMenu_Constellation:	//��������ͼ
	{
		//���������Ӵ����ҵ������˵�������
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWnd() == GetHWD())
			{
				it->first->GetPlotObj()->GetDspObj()->QAM_Demod(6, 0.2, 1);
				it->first->GetPlotObj()->SetDrawType(en_Draw_Constellation);
			}		

		}
		break;
	}
	case 4:
		MessageBox(NULL, _T("����״̬��æµ"), NULL, MB_OK);
		break;
	default:
		break;
	}
}

