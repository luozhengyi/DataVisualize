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

	//1.绘制背景(填充背景)
	//GDI+的固体刷子，这个刷子比较强大，可以渐变，还可以用图片做刷子;Color中的第一个参数表示透明度,255表示不透明
	Gdiplus::SolidBrush sbrsh(Gdiplus::Color(200, 255,255, 255));
	
	if (GetItemStatus() == E_ItemStatus_Over)	//鼠标移到了UI Item上
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

	//2.绘制 Item
	Gdiplus::Image img((GetItemRSCPath())->c_str());
	Gdiplus::Rect itemRECT;
	//让图标垂直居中
	itemRECT.X = GetItemRECT().left+6;
	itemRECT.Y = GetItemRECT().top;
	itemRECT.Width = GetItemRECT().right - GetItemRECT().left-12;
	itemRECT.Height = GetItemRECT().bottom - GetItemRECT().top;

	//3.绘制分割线
	Gdiplus::Pen newpen(Gdiplus::Color(255, 255, 255), 1);
	grx.DrawLine(&newpen, GetItemRECT().right, GetItemRECT().top, GetItemRECT().right, GetItemRECT().bottom);


	Gdiplus::Status sta = grx.DrawImage(&img, itemRECT);
}

/************************************************************************/

void CXUITool::AddItem(int iItemID, const wchar_t* wszItemRSCPath, const wchar_t* wszCaption)
{
	CXUIItemBase *pUIItem = new CXUIToolItem();
	pUIItem->SetItemID(iItemID);				//Item ID
	pUIItem->SetItemRSCPath(wszItemRSCPath);	//图标路径
	pUIItem->SetItemCaption(wszCaption);		//提示语

	//计算UI项在窗口中的区域:横着排列
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
	//从模板中拷贝的代码
	PAINTSTRUCT ps;
	HDC hdc = GetDC(GetHWD());

	//计算菜单在窗口中的位置
	//GetClientRect(m_hWnd, &m_rcMenu);
	RECT XUIRect;
	XUIRect.top = 0;
	XUIRect.left = 0;
	XUIRect.right = GetItemWidth() * GetItemMapSize();
	XUIRect.bottom = GetItemHeigh();
	//其实这里 rect.left = rect.top = 0;
	int XUIWidth = XUIRect.right - XUIRect.left;
	int XUIHeight = XUIRect.bottom - XUIRect.top;
	//记录UI所在的rect
	SetXUIRect(XUIRect);

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP hBmp = CreateCompatibleBitmap(hdc, XUIWidth, XUIHeight);
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(memDC, hBmp);
	//绘制代码


	
	
	//2.绘制菜单项
	Gdiplus::Graphics grx(memDC);
	ItemMap::iterator it = GetItemMap().begin();
	while (it != GetItemMap().end())
	{
		(*it).second->Draw(memDC);	//it->first表示键；it->second表示值
		++it;
	}
	
	//绘制提示语 tooltip
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
		

	//拷贝内存DC到窗口DC上
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

	//判断光标在不在UI上
	if (!PtInRect(&(GetXUIRect()), pt))
	{
		if (GetItemOver() != NULL)
			GetItemOver()->SetItemStatus(E_ItemStatus_Normal);
		SetItemOver(NULL);
		InvalidateRect(GetHWD(), &(GetXUIRect()), FALSE);		//让UI rect刷新重绘 发出WM_PAINT消息
		return;
	}
		

		
		
	//判断光标在那个UI项上
	CXUIItemBase* pItem = FindItem(pt);
	if (pItem && pItem !=GetItemOver())		//鼠标在本身不是高亮的菜单项上
	{
		pItem->SetItemStatus(E_ItemStatus_Over);
		if (GetItemOver() != NULL)
			GetItemOver()->SetItemStatus(E_ItemStatus_Normal);
		SetItemOver(pItem);
		InvalidateRect(GetHWD(), &(GetXUIRect()), FALSE);		//让UI rect刷新重绘 发出WM_PAINT消息
	}

}

void CXUITool::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	//获取鼠标坐标
	POINT pt;
	::GetCursorPos(&pt);
	::ScreenToClient(GetHWD(), &pt);

	//获取窗口客户区坐标
	//RECT rectClient;
	//::GetClientRect(GetHWD(), &rectClient);

	//判断这个鼠标点在不在UI区域
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
		//遍历所有子窗口，找到XUI所属的子窗口
		HWND hWnd = NULL;
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWndID() == E_WND_Plot_Right)		//让右边窗口隐藏，其实是让左边窗口遮盖它
			{
				it->first->SetWndStatus(E_WND_Show);
				SendMessage(it->first->GetWnd(), WM_SIZE, 0, 0);
			}
			if (it->first->GetWndID() == E_WND_Plot_Left)
				hWnd = it->first->GetWnd();

		}
		SendMessage(hWnd, WM_SIZE, 0, 0);	//放在后面防止右边窗口的状态还没改变
		break;
	}

		break;
	case E_XUITOOL_MERGE:
	{
		//遍历所有子窗口，找到XUI所属的子窗口
		HWND hWnd = NULL;
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWndID() == E_WND_Plot_Right)		//让右边窗口隐藏，其实是让左边窗口遮盖它
			{
				it->first->SetWndStatus(E_WND_Hide);
				SendMessage(it->first->GetWnd(), WM_SIZE, 0, 0);
			}
			if (it->first->GetWndID() == E_WND_Plot_Left) //放在后面防止右边窗口的状态还没改变
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

	//1.绘制背景(填充背景)
	//GDI+的固体刷子，这个刷子比较强大，可以渐变，还可以用图片做刷子;Color中的第一个参数表示透明度,255表示不透明
	Gdiplus::SolidBrush sbrsh(Gdiplus::Color(255, 230, 230, 230));	//白色

	if (GetItemStatus() == E_ItemStatus_Over)	//鼠标移到了UI Item上
	{
		sbrsh.SetColor(Gdiplus::Color(255, 192, 192, 192));				//蓝色
	}
	grx.FillRectangle(&sbrsh, Gdiplus::Rect(GetItemRECT().left, GetItemRECT().top, iItemWidth, iItemHeight));
	//grx.FillRectangle(&sbrsh, Gdiplus::Rect(0, 0, iItemWidth, iItemHeight));

	//2.绘制 Item 菜单项
	Gdiplus::RectF textRECT;
	//让文字左对齐，居中
#define ICON_WIDTH 32
	textRECT.X = GetItemRECT().left + ICON_WIDTH;
	textRECT.Y = GetItemRECT().top;
	textRECT.Width = GetItemRECT().right - textRECT.X;
	textRECT.Height = GetItemRECT().bottom - GetItemRECT().top;


	//文字字体
	Gdiplus::FontFamily fontFamily(_T("宋体"));		//设置字体
	Gdiplus::Font font(&fontFamily, 12);			//设置字体大小
	Gdiplus::StringFormat strFormat;				//设置文字对其风格
	strFormat.SetAlignment(Gdiplus::StringAlignmentNear);		//横向居左
	strFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);	//纵向居中

	//用画刷将文字画上;黑色
	Gdiplus::SolidBrush fontBrush(Gdiplus::Color(0, 0, 0));
	grx.DrawString(GetItemCaption()->c_str(), wcslen(GetItemCaption()->c_str()), &font, textRECT, &strFormat, &fontBrush);


	//3.绘制分割线,
	Gdiplus::Pen newpen(Gdiplus::Color(255, 255, 255), 1);
	Gdiplus::Status sta = grx.DrawLine(&newpen, GetItemRECT().left + ICON_WIDTH, GetItemRECT().top, GetItemRECT().left + ICON_WIDTH, GetItemRECT().bottom);
	grx.DrawLine(&newpen, GetItemRECT().left + ICON_WIDTH, GetItemRECT().top,
						  GetItemRECT().right,             GetItemRECT().top);
	grx.DrawLine(&newpen, GetItemRECT().left + ICON_WIDTH, GetItemRECT().bottom,
						  GetItemRECT().right            , GetItemRECT().bottom);

	//1.绘制边框
#define FRAMEWIDTH 1
	newpen.SetColor(Gdiplus::Color(128, 128, 128));
	grx.DrawLine(&newpen, FRAMEWIDTH, GetItemRECT().top,
						  FRAMEWIDTH, GetItemRECT().bottom);		//左边框
	grx.DrawLine(&newpen, iItemWidth - 2 * FRAMEWIDTH, GetItemRECT().top,
						  iItemWidth - 2 * FRAMEWIDTH, GetItemRECT().bottom);							//右边框




}

/************************************************************************/
void CXUIMenu::AddItem(int iItemID, const wchar_t* wszItemRSCPath, const wchar_t* wszCaption)
{
	CXUIItemBase *pUIItem = new CXUIMenuItem();
	pUIItem->SetItemID(iItemID);
	pUIItem->SetItemCaption(wszCaption);

	//计算菜单项在内存memDC中的位置：竖着排列；最后再贴到物理DC上的
	//UI item 的RECT是相对内存DC的，UI的RECT是相对物理DC的
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

	//从模板中拷贝的代码
	PAINTSTRUCT ps;
	HDC hdc = GetDC(GetHWD());

	//计算菜单在窗口中的位置,竖着排列
	//GetClientRect(m_hWnd, &m_rcMenu);
	RECT XUIRect;
	XUIRect.top = GetPos().y;
	XUIRect.left = GetPos().x;
	XUIRect.right = XUIRect.left + GetItemWidth();
	XUIRect.bottom = XUIRect.top + GetItemHeigh()* GetItemMapSize();
	//其实这里 rect.left = rect.top = 0;
	int XUIWidth = XUIRect.right - XUIRect.left;
	int XUIHeight = XUIRect.bottom - XUIRect.top;
	//记录UI所在的rect
	SetXUIRect(XUIRect);

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP hBmp = CreateCompatibleBitmap(hdc, XUIWidth, XUIHeight);
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(memDC, hBmp);
	//绘制代码
	Gdiplus::Graphics grx(memDC);


	//2.绘制菜单项
	
	ItemMap::iterator it = GetItemMap().begin();
	while (it != GetItemMap().end())
	{
		(*it).second->Draw(memDC);	//it->first表示键；it->second表示值
		++it;
	}

	//拷贝内存DC到窗口DC上
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

	//判断光标在不在UI上
	if (!PtInRect(&(GetXUIRect()), pt))
		return;

	//判断光标在那个UI项上
	//将pt在物理DC上的位置转换为内存DC上的位置
	pt.x -= GetXUIRect().left;
	pt.y -= GetXUIRect().top;
	CXUIItemBase* pItem = FindItem(pt);
	if (pItem && pItem != GetItemOver())		//鼠标在本身不是高亮的菜单项上
	{
		pItem->SetItemStatus(E_ItemStatus_Over);
		if (GetItemOver() != NULL)
			GetItemOver()->SetItemStatus(E_ItemStatus_Normal);
		SetItemOver(pItem);
		//InvalidateRect(GetHWD(), &(GetXUIRect()), FALSE);		//让UI rect刷新重绘
	}

}

void CXUIMenu::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	//获取鼠标坐标
	POINT pt;
	//::GetCursorPos(&pt);
	//::ScreenToClient(GetHWD(), &pt);
	pt = *(POINT*)wParam;

	//获取窗口客户区坐标
	//RECT rectClient;
	//::GetClientRect(GetHWD(), &rectClient);

	

	//判断这个鼠标点在不在UI区域
	if (PtInRect(&(GetXUIRect()),pt))
	{
		//将pt在物理DC上的位置转换为内存DC上的位置
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
	case E_XUIMenu_TimeDomain:		//绘制时域
	{
		//遍历所有子窗口，找到XUI所属的子窗口
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWnd() == GetHWD())
				it->first->GetPlotObj()->SetDrawType(en_Draw_Orignal);
		}
		break;
	}
	case E_XUIMenu_FreqDomain:		//绘制频域
	{
		//遍历所有子窗口找到弹出菜单所属的
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		for (; it != g_WndManager.GetChildWndMap()->end(); it++)
		{
			if (it->first->GetWnd() == GetHWD())
				it->first->GetPlotObj()->SetDrawType(en_Draw_Spectrum);

		}
		break;
	}
	case E_XUIMenu_Constellation:	//绘制星座图
	{
		//遍历所有子窗口找到弹出菜单所属的
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
		MessageBox(NULL, _T("您的状态是忙碌"), NULL, MB_OK);
		break;
	default:
		break;
	}
}

