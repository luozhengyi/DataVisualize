#ifndef __GDIPLUSUSE_H__
#define __GDIPLUSUSE_H__
#include "gdiplus.h."
#include <string>
#include <map>

#pragma comment(lib,"Gdiplus.lib")


class CXUIBase;

enum enItemStatus
{
	E_ItemStatus_Normal = 1,	//正常状态
	E_ItemStatus_Over = 2		//鼠标移上状态
};

enum enXUIStatus
{
	E_XUI_Invisible = 0,
	E_XUI_Visible = 1
};

enum enToolItemID
{
	E_XUITOOL_START = 1,
	E_XUITOOL_PAUSE = 2,
	E_XUITOOL_ZOOMIN = 3,
	E_XUITOOL_ZOOMOUT = 4,
	E_XUITOOL_MOVE = 5,
	E_XUITOOL_SPLIT = 6,
	E_XUITOOL_MERGE = 7,
	E_XUITOOL_NETCONN = 8
};

enum enMenuItemID
{
	E_XUIMenu_TimeDomain = 1,
	E_XUIMenu_FreqDomain = 2,
	E_XUIMenu_Constellation = 3,
};


class CXUIItemBase		//Item 基类
{
public:
	CXUIItemBase(){
	}
	virtual ~CXUIItemBase(){
	};

private:
	int  m_iItemID;			//Item ID
	RECT m_itemRECT;		//Item 所在的RECT
	int m_ItemRSCID;		//Item 资源ID
	std::wstring m_wszCaption;			//Item caption
	std::wstring m_wszItemRSCPath;		//Item 资源路径
	enItemStatus m_enItemStatus;		//UI item 状态
	
public:
	void SetItemID(int iItemID){ 
		m_iItemID = iItemID;
	};
	int GetItemID(){
		return m_iItemID;
	};
	void SetItemRECT(const RECT &itemRECT){ 
		m_itemRECT = itemRECT; 
	};
	RECT& GetItemRECT(){
		return m_itemRECT;
	};
	void SetItemCaption(const wchar_t* wszCaption){ 
		m_wszCaption = wszCaption; 
	};
	std::wstring* GetItemCaption(){ 
		return &m_wszCaption; 
	};
	void SetItemRSCPath(const wchar_t* wszItemRSCPath){
		m_wszItemRSCPath = wszItemRSCPath;
	};
	std::wstring* GetItemRSCPath(){
		return &m_wszItemRSCPath;
	};
	void SetItemRSCID(int ItemRSCID){
		m_ItemRSCID = ItemRSCID;
};
	int GetItemRSCID(){
		return m_ItemRSCID;
	};
	void SetItemStatus(enItemStatus enStatus){
		m_enItemStatus = enStatus;
	};
	enItemStatus GetItemStatus(){
		return m_enItemStatus;
	};
	
public:
	BOOL HitTest(const POINT& pt){
		//此时pt是在内存DC上的坐标
		if (PtInRect(&m_itemRECT,pt))
			return TRUE;
		return FALSE;
	};
public:
	virtual void Draw(HDC hdc) =0;
		

};



class CXUIBase
{
	typedef std::map<int, CXUIItemBase*> ItemMap;
#define WM_XUI_MSG 50

public:
	CXUIBase(){
		m_toolTip = NULL;
	};
	virtual ~CXUIBase(){
		ClearItem();
		if (m_toolTip)
			DestroyWindow(m_toolTip);
	};

public:
	void InitUI(int iItemWidth, int iItemHeight, HWND hWnd){
		ClearItem();
		m_iItemWidth = iItemWidth;
		m_iItemHeigh = iItemHeight;
		m_hWnd = hWnd;
		SetItemOver(NULL);
	};
	void ClearItem(){
		ItemMap::iterator it = m_ItemMap.begin();
		while (it != m_ItemMap.end())
		{
			delete (*it).second;
			++it;
		}
		m_ItemMap.clear();
	};
	void InsertItemMap(int iItemID, CXUIItemBase* pCUXItem){
		m_ItemMap[iItemID] = pCUXItem;
	}
	ItemMap& GetItemMap(){
		return m_ItemMap;
	};
	unsigned int GetItemMapSize(){
		return m_ItemMap.size();
	}
	void SetItemWidth(int iItemWidth){ 
		m_iItemWidth = iItemWidth; 
	};
	int GetItemWidth(){
		return m_iItemWidth;
	};
	void SetItemHeigh(int iItemHeigh){ 
		m_iItemHeigh = iItemHeigh; 
	};
	int GetItemHeigh(){ 
		return m_iItemHeigh; 
	};
	HWND GetHWD(){
		return m_hWnd;
	};
	void SetXUIRect(RECT XUIRECT){ 
		m_XUIRECT = XUIRECT; 
	};
	const RECT GetXUIRect(){
		return m_XUIRECT;
	}
	void SetItemOver(CXUIItemBase* pItemOver)	{
		m_pItemOver = pItemOver;
	};
	CXUIItemBase* GetItemOver(){
		return m_pItemOver;
	}
	CXUIItemBase* FindItem(POINT pt){
		CXUIItemBase* pItem = NULL;
		ItemMap::iterator it = m_ItemMap.begin();
		while (it != m_ItemMap.end())
		{
			if ((*it).second->HitTest(pt))
			{
				pItem = (*it).second;
				break;
			}
			++it;
		}
		return pItem;
	};	//根据光标点查找UI项
public:
	void SetXUIStatus(enXUIStatus XUIStatus){
		m_enXUIStatus = XUIStatus;
	};
	enXUIStatus GetXUIStatus(void){
		return m_enXUIStatus;
	};

public:
	void SetPos(POINT pt){
		m_pt = pt;
	};
	POINT GetPos(){
		return m_pt;
	};
public:
	HWND GetToolTipWnd(){
		return m_toolTip;
	};
	void SetToolTipWnd(HWND hMainWnd){
		if (hMainWnd != NULL)
			m_toolTip = CreateWindow(TEXT("STATIC"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 0, 0, hMainWnd, (HMENU)10000, NULL, NULL);
	}
public:
	/*
	*@Param  iItemID：ui项ID；itemRSCID：ui项资源ID；wszItemRSCPath：ui项资源路径；wszCaption：资源向标题
	*/
	//virtual void AddItem(int iItemID, int ItemRSCID, const wchar_t* wszCaption);					//添加UI项
	virtual void AddItem(int iItemID, const wchar_t* wszItemRSCPath, const wchar_t* wszCaption)=0;	//添加UI项
	virtual void OnPaint() = 0;						//绘制UI
	virtual void OnMouseMove(WPARAM wParam, LPARAM lParam) = 0;
	virtual void OnLButtonDown(WPARAM wParam, LPARAM lParam) = 0;
	virtual void OnXUIMSG(WPARAM wParam, LPARAM LPARAM) = 0;

private:
	HWND m_hWnd;			//UI 所在的父窗口
	ItemMap m_ItemMap;		//UI 项与UI项ID的映射
	int m_iItemWidth;		//UI 项的宽度
	int m_iItemHeigh;		//UI 项的高度
	RECT m_XUIRECT;			//UI 所在矩形区域
	CXUIItemBase* m_pItemOver;	//当前高亮的UI item项
	HWND m_toolTip;				//实时提示窗口
private:
	enXUIStatus m_enXUIStatus;
private:
	POINT m_pt;		//鼠标右键点下的位置；为弹出式菜单服务的

};




class CXUIToolItem :public CXUIItemBase
{
public:
	CXUIToolItem(){};
	virtual ~CXUIToolItem(){};
public:
	virtual void Draw(HDC hdc);
};

class CXUITool:public CXUIBase
{
public:
	CXUITool(){
		SetXUIStatus(E_XUI_Visible);
	};
	virtual ~CXUITool(){};

public:
	virtual void AddItem(int iItemID, const wchar_t* wszItemRSCPath, const wchar_t* wszCaption);	//添加UI项
	virtual void OnPaint();						//绘制菜单
	virtual void OnMouseMove(WPARAM wParam, LPARAM lParam);
	virtual void OnLButtonDown(WPARAM wParam, LPARAM lParam);
	virtual void OnXUIMSG(WPARAM wParam, LPARAM LPARAM);
private:


};


class CXUIMenuItem :public CXUIItemBase
{
public:
	CXUIMenuItem(){};
	virtual ~CXUIMenuItem(){};
public:
	virtual void Draw(HDC hdc);
};

class CXUIMenu :public CXUIBase
{
public:
	CXUIMenu(){ 
		SetXUIStatus(E_XUI_Invisible);
	};
	virtual ~CXUIMenu(){};

public:
	virtual void AddItem(int iItemID, const wchar_t* wszItemRSCPath, const wchar_t* wszCaption);	//添加UI项
	virtual void OnPaint();						//绘制菜单
	virtual void OnMouseMove(WPARAM wParam, LPARAM lParam);
	virtual void OnLButtonDown(WPARAM wParam, LPARAM lParam);
	virtual void OnXUIMSG(WPARAM wParam, LPARAM LPARAM);

};


#endif	//__GDIPLUSUSE_H__