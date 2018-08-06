#ifndef __WNDMANAGER_H__
#define __WNDMANAGER_H__
#include <map>
#include "windows.h"

class CPlot;
class CXUIBase;


enum enChildWndID
{
	E_WND_Tool = 1,
	E_WND_Plot_Left = 2,
	E_WND_Plot_Right =3,
};

enum enWndStatus
{
	E_WND_Show = 1,
	E_WND_Hide = 2
};


struct MYRECT{
	int x;
	int y;
	int width;
	int heigh;
	MYRECT()
	{
		x = 0;
		y = 0;
		width = 0;
		heigh = 0;
	};
	MYRECT(const RECT& rect)
	{
		x = rect.left;
		y = rect.top;
		width = rect.right - rect.left;
		heigh = rect.bottom - rect.top;
	};
	~MYRECT(){};
	MYRECT& operator= (const RECT& rect)
	{
		this->MYRECT::MYRECT(rect);		//��ȷ
		return *this;
	};

};


class CChildWnd
{
public:
	CChildWnd(){
		m_enWndStatus = E_WND_Show;
	};
	virtual ~CChildWnd(){
		if (m_plot)
			delete m_plot;
		if (m_xui)
			delete m_xui;
	};
private:
	CPlot* m_plot;
	CXUIBase* m_xui;
	HWND m_hWnd;				//�Ӵ��ھ��
	int m_iWndID;				//�Ӵ���ID
	enWndStatus m_enWndStatus;	//�Ӵ��ڵ�״̬
	//RECT m_WndRect;			//�Ӵ��ڵľ�������
public:
	void SetPlotObj(CPlot* pPlot) {
		m_plot = pPlot;
	}
	CPlot* GetPlotObj(){		//ָ�������
		return m_plot;
	}
public:
	void SetXUI(CXUIBase* pxui){
		m_xui = pxui;
	}
	CXUIBase*  GetXUI(){
		return m_xui;
	};
public:
	void SetWnd(HWND hWnd){
		m_hWnd = hWnd;
	};
	HWND GetWnd(){
		return m_hWnd;
	}
public:
	void SetWndID(int wndID){
		m_iWndID = wndID;
	};
	int GetWndID(){
		return m_iWndID;
	}
public:
	void SetWndStatus(enWndStatus wndStatus){
		m_enWndStatus = wndStatus;
	}
	enWndStatus GetWndStatus()	{
		return m_enWndStatus;
	};
public:
	void OnCreate(CPlot* pPlot, CXUIBase* pXUI){
		SetPlotObj(pPlot);
		SetXUI(pXUI);
	};
	//BOOL WndProc();
};



typedef LRESULT(*wndproc)(CChildWnd* pChildWnd, UINT message, WPARAM wParam, LPARAM lParam);
typedef std::map<CChildWnd*, wndproc> childwndmap;

class CWndManager
{
public:
	CWndManager(){};
	virtual ~CWndManager(){
		childwndmap::iterator it = m_ChildWndMap.begin();
		while (it != m_ChildWndMap.end())
		{
			delete it->first;
			++it;
		}
		m_ChildWndMap.clear();
	};
public:
	BOOL AddChildWnd(int wndID, RECT childRect, wndproc pWndProc);
public:
	void Init(HWND hClientWnd){
		m_hClientWnd = hClientWnd;
	};
public:
	HWND GetClientWnd(){
		return m_hClientWnd;
	};
	childwndmap* GetChildWndMap(){
		return &m_ChildWndMap;
	};

private:
	HWND m_hClientWnd;
private:
	/*
	 *@Typename  int������ID��
	 */
	//std::map<int, CChildWnd*> m_ChildWndMap;
	childwndmap	m_ChildWndMap;


};



LRESULT ToolWndProc(CChildWnd* pChildWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT PlotWndProc(CChildWnd* pChildWnd, UINT message, WPARAM wParam, LPARAM lParam);




#endif	//__WNDMANAGER_H__