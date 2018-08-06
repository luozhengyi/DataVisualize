#include "stdafx.h"
#include "WinUser.h"
#include "stdlib.h"
#include "string.h"
#include "resource.h"
#include "DataVisualize_Demo.h"
#include "Plot.h"
#include <GL/gl.h>  
#include <GL/glu.h>  
#include <GL/glaux.h> 
#include "gdiPlusUse.h"
#include "WndManager.h"

#define MAX_LOADSTRING 100



//定义全局变量
HINSTANCE hInst;						//全局实例句柄
TCHAR szTitle[MAX_LOADSTRING];			//标题栏名称
TCHAR szWindowClass[MAX_LOADSTRING];	//主窗口类名
HWND g_MainWnd = NULL;					//主窗口句柄

CWndManager g_WndManager;				//窗口管理器
TCHAR szChildClass[MAX_LOADSTRING];		//子窗口类名
HWND g_hMDIClientWnd = NULL;			//客户区窗口句柄
RECT g_ClientRect;						//客户窗口
MYRECT g_ClientMyRect;					//客户窗口
#define  TOOLS_HIGH 40					//上面子窗口的占空比
int ErrorCode;

////
HWND g_Edit_FrameNum = nullptr;			//数据采集：FrameNum文本框句柄

// GDI+初始化需要的变量、和自己的变量
Gdiplus::GdiplusStartupInput g_Gdiplus = NULL;
ULONG_PTR g_GdiToken = NULL;

//  画图操作自定义变量
tagPOINT pt1 = { 0, 0 };
tagPOINT pt2 = { 0, 0 };
BOOL bLBDown = false;

//函数声明
ATOM MyRegisterClass(HINSTANCE);								//注册主窗口类和子窗口类
LRESULT CALLBACK MDIFrameWndProc(HWND, UINT, WPARAM, LPARAM);	//主窗口的过程函数
LRESULT CALLBACK MDIChildWndProc(HWND, UINT, WPARAM, LPARAM);	//子窗口的过程函数
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); //"关于"窗口回调函数
INT_PTR CALLBACK DataAcq(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); //"数据采集"窗口回调函数
BOOL InitInstance(HINSTANCE, int);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;


	//GDI+初始化
	Gdiplus::GdiplusStartup(&g_GdiToken, &g_Gdiplus, NULL);

	// Initialize global strings
	memcpy(szTitle, L"Scope", MAX_LOADSTRING);
	memcpy(szWindowClass, L"MyWndClass", MAX_LOADSTRING);
	memcpy(szChildClass, L"MDIChildWndClass", MAX_LOADSTRING);


	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//关闭GDI+环境
	Gdiplus::GdiplusShutdown(g_GdiToken);

	return (int)msg.wParam;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // 将实例句柄存储在全局变量中


	//1、窗口类名称
	//2、窗口标题
	//3、窗口的样式
	g_MainWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | MDIS_ALLCHILDSTYLES,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	hWnd = g_MainWnd;

	if (!hWnd)
	{
		return FALSE;
	}
	

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	/****************************************注册主窗口类*****************************************/
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)MDIFrameWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DATAVISUALIZE_DEMO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//自己创建的菜单，修改菜单ID时，头文件和资源文件里面都要改
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_DATAVISUALIZE_DEMO);;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if (!RegisterClassEx(&wcex))
		return FALSE;

	/****************************************注册子窗口类*****************************************/
	wcex.lpfnWndProc = (WNDPROC)MDIChildWndProc;
	wcex.hIcon = NULL; //LoadIcon(hInst, IDNOTE);
	wcex.lpszMenuName = (LPCTSTR)NULL;
	wcex.cbWndExtra = NULL;// CBWNDEXTRA;
	wcex.lpszClassName = szChildClass;

	return RegisterClassEx(&wcex);
}


/*
* @fn: MDIFrameWndProc
* @Remarks: 主窗口过程函数
*/
LRESULT CALLBACK MDIFrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmID, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	CLIENTCREATESTRUCT MDIClientCreateStruct;

	switch (message)
	{
	case WM_CREATE:
	{
		GetClientRect(hWnd, &g_ClientRect);
		g_ClientMyRect = g_ClientRect;

		// 在主窗口的 WM_CREATE 消息中创建客户窗口(客户窗口是系统自动注册的)
		MDIClientCreateStruct.hWindowMenu = NULL;
		MDIClientCreateStruct.idFirstChild = 1024;
		g_hMDIClientWnd = CreateWindowEx(NULL,
			TEXT("MDICLIENT"), // 系统预定义的类名
			NULL,
			WS_CHILD | MDIS_ALLCHILDSTYLES,
			g_ClientRect.left, g_ClientRect.top,
			g_ClientRect.right - g_ClientRect.left,
			g_ClientRect.bottom - g_ClientRect.top,
			hWnd, (HMENU)0xCAC, hInst, (void*)&MDIClientCreateStruct);


		g_WndManager.Init(g_hMDIClientWnd);


		RECT newChildRect = { g_ClientRect.left, g_ClientRect.top, g_ClientRect.right, g_ClientRect.top + TOOLS_HIGH };
		g_WndManager.AddChildWnd(E_WND_Tool, newChildRect, ToolWndProc);	//创建工具栏子窗口


		newChildRect = { g_ClientRect.left, g_ClientRect.top + TOOLS_HIGH, g_ClientRect.right/2, g_ClientRect.bottom };
		g_WndManager.AddChildWnd(E_WND_Plot_Left, newChildRect, PlotWndProc);	//创建左边绘图子窗口

		newChildRect = { g_ClientRect.left+g_ClientRect.right / 2, g_ClientRect.top + TOOLS_HIGH, g_ClientRect.right, g_ClientRect.bottom };
		g_WndManager.AddChildWnd(E_WND_Plot_Right, newChildRect, PlotWndProc);	//创建右边绘图子窗口

		ShowWindow(g_hMDIClientWnd, SW_SHOW);
		break;
	}
	case WM_SIZE:
	{
		GetClientRect(hWnd, &g_ClientRect);
		g_ClientMyRect = g_ClientRect;
		MoveWindow(g_hMDIClientWnd, g_ClientMyRect.x, g_ClientMyRect.y, g_ClientMyRect.width, g_ClientMyRect.heigh, true);
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			SendMessage((*it).first->GetWnd(), WM_SIZE, 0, 0);
			++it;
		}
		break;
	}

	case WM_COMMAND:
	{
		wmID = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmID)
		{
		case IDM_OPEN:
		{
			OPENFILENAME stOpenFile;
			memset(&stOpenFile, 0, sizeof(OPENFILENAME));	//如果初始化会弹出对话框失败
			//wchar_t wcFilePath[MAX_PATH] = _T("F:\\Program\\择善教育\\蔡\\MapEditor\\Debug");
			wchar_t wcFilePath[MAX_PATH] = { _T("") };
			stOpenFile.lpstrInitialDir = _T(".\\");
			stOpenFile.lpstrFile = wcFilePath;
			stOpenFile.nMaxFile = MAX_PATH;
			stOpenFile.lpstrFilter = L"*.bin\0\0";		//必须要以两个NULL结尾
			stOpenFile.lpstrDefExt = L"bin";
			stOpenFile.lStructSize = sizeof(OPENFILENAME);
			stOpenFile.hwndOwner = hWnd;
			//stOpenFile.Flags = stOpenFile.Flags | OFN_ENABLEHOOK;
			//stOpenFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
			//stOpenFile.lpfnHook = (LPOFNHOOKPROC)DataAcq;
			if (GetOpenFileName(&stOpenFile))	//弹出对话框
			{
				CPlot::GetDataPath(stOpenFile.lpstrFile);
				CPlot::SetDataSource(en_From_File);
				CPlot::SetDrawStatus(E_Draw_Fresh);
			}
			break;
		}
		case IDM_SAMPLE:
		{
			OPENFILENAME stSaveFile;
			memset(&stSaveFile, 0, sizeof(OPENFILENAME));	//如果初始化会弹出对话框失败
			//wchar_t wcFilePath[MAX_PATH] = _T("F:\\Program\\择善教育\\蔡\\MapEditor\\Debug");
			wchar_t wcFilePath[MAX_PATH] = { _T("未命名") };
			stSaveFile.lpstrInitialDir = _T(".\\");
			stSaveFile.lpstrFile = wcFilePath;
			stSaveFile.nMaxFile = MAX_PATH;
			stSaveFile.lpstrFilter = L"*.bin\0\0";
			stSaveFile.lpstrDefExt = L"bin";
			stSaveFile.lStructSize = sizeof(OPENFILENAME);
			stSaveFile.hwndOwner = hWnd;
			//stSaveFile.Flags = stOpenFile.Flags | OFN_ENABLEHOOK;
			stSaveFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
			stSaveFile.lpfnHook = (LPOFNHOOKPROC)DataAcq;
			if (GetSaveFileName(&stSaveFile))	//弹出对话框
			{
				
				CPlot::GetstFile().filePath = stSaveFile.lpstrFile;
				if(CPlot::GetstFile().Init_stFile())
					CPlot::SetDrawStatus(E_Draw_Static);
				
			}
			break;
		}
			
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
			
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		default:
			DefFrameProc(hWnd, g_hMDIClientWnd, message, wParam, lParam);	//处理未处理的消息
			break;
		}


		break;
	}
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO:  在此添加任意绘图代码...
		EndPaint(hWnd, &ps);
		break;


	case WM_ACTIVATE:	//窗口失去或者获得焦点
	{
		//通知子窗口失去或者获得焦点
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			SendMessage((*it).first->GetWnd(), WM_ACTIVATE, 0, 0);
			++it;
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		//return DefWindowProc(hWnd, message, wParam, lParam);					//SDI
		return DefFrameProc(hWnd, g_hMDIClientWnd, message, wParam, lParam);	//MDI
	}

	return 0;
}

LRESULT CALLBACK MDIChildWndProc(HWND hWndChild, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmID, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
	for (; it != g_WndManager.GetChildWndMap()->end(); ++it)
	{
		if (it->first->GetWnd() == hWndChild)
			it->second(it->first,message,wParam,lParam);
	}

	switch (message)
	{

	default:
		//return DefWindowProc(hWndChild, message, wParam, lParam);					//SDI
		return DefMDIChildProc(hWndChild, message, wParam, lParam);					//MDI
	}

	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// “数据采集”框的消息处理程序。
INT_PTR CALLBACK DataAcq(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hDlg);
	RECT rect;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		//在文档打开文档的下面增长200个像素，左上点坐标与宽度不起作用
		MoveWindow(hDlg, 0, 0, 800, 200, true);	
		//默认的窗口大小是：758*576；经上改变后为：758*776
		GetClientRect(hDlg, &rect);
		

		CreateWindow(TEXT("STATIC"), TEXT("DataWidth"), WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 0, 120, 25, hDlg, (HMENU)0x100, NULL, NULL);
		CreateWindow(TEXT("STATIC"), TEXT("2byte"), WS_CHILD | WS_VISIBLE | WS_BORDER, 400, 0, 120, 25, hDlg, (HMENU)0x150, NULL, NULL);

		CreateWindow(TEXT("STATIC"), TEXT("FrameLen"), WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 50, 120, 25, hDlg, (HMENU)0x200, NULL, NULL);
		CreateWindow(TEXT("STATIC"), TEXT("262144"), WS_CHILD | WS_VISIBLE | WS_BORDER, 400, 50, 120, 25, hDlg, (HMENU)0x250, NULL, NULL);
		
		CreateWindow(TEXT("STATIC"), TEXT("FrameNum"), WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 100, 120, 25, hDlg, (HMENU)0x300, NULL, NULL);
		g_Edit_FrameNum = CreateWindow(TEXT("EDIT"), TEXT("82"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 400, 100, 120, 25, hDlg, (HMENU)0x350, NULL, NULL);


		wchar_t szText[100];
		memset(szText, 0, sizeof(szText));
		GetWindowText(g_Edit_FrameNum, szText, 99);
		CPlot::GetstFile().frameNum = _wtoi(szText);

		return (INT_PTR)TRUE;
	}
	case WM_SIZE:
		break;
	case WM_PAINT:
		hdc = BeginPaint(hDlg, &ps);
		//::MoveToEx(hdc, 0, 520, NULL);
		//::LineTo(hdc, 800, 520);
		EndPaint(hDlg, &ps);
	case WM_COMMAND:
	{
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择: 
		switch (wmId)
		{
		case 0x350:		//FrameNum对应的文本框
		{
			wchar_t szText[100];
			memset(szText, 0, sizeof(szText));
			GetWindowText(g_Edit_FrameNum, szText, 99);
			CPlot::GetstFile().frameNum = _wtoi(szText);
			break;
		}
		default:
			break;
		}

		break;
	}
		
	}
	return (INT_PTR)FALSE;
}