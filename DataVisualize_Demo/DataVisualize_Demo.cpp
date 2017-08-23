// DataVisualize_Demo.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "DataVisualize_Demo.h"
#include "Plot.h"
#include <GL/gl.h>  
#include <GL/glu.h>  
#include <GL/glaux.h>  

#define MAX_LOADSTRING 100


// 全局变量: 
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名
/*  自定义变量  */
CPlot g_plot;
tagPOINT pt1 = { 0, 0 };
tagPOINT pt2 = { 0, 0 };
bool bLBDown = false;


// 此代码模块中包含的函数的前向声明: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO:  在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_DATAVISUALIZE_DEMO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DATAVISUALIZE_DEMO));

	// 主消息循环: 
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DATAVISUALIZE_DEMO));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_DATAVISUALIZE_DEMO);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;


	switch (message)
	{
	/************************************自己的消息*****************************/
	case WM_CREATE:
	{
		g_plot.Init(hWnd);						//初始化OpenGL绘图环境
		break;
	}
	case WM_LBUTTONDOWN:
	{
		GetCursorPos(&pt1);
		ScreenToClient(hWnd, &pt1);    // 转屏幕座标,如果用屏幕扩展就会出错

		if (g_plot.IsinRect(pt1))
			bLBDown = true;
		else
			bLBDown = false;
		

		/*RECT rect = { 100, 100, 300, 300 };
		DrawText(GetDC(hWnd), L"Hello", strlen("Hello"), &rect, 0);
		MessageBox(NULL, L"Hello", L"test", MB_OK);*/
		break;
	}

	case WM_MOUSEMOVE:
	{
		if (bLBDown)
		{
			GetCursorPos(&pt2);
			ScreenToClient(hWnd, &pt2);		//如果用屏幕扩展就会出错

			if (g_plot.IsinRect(pt1))
			{
				g_plot.OnTranslate(pt1, pt2);
				pt1 = pt2;	//很关键，鼠标一直按着不放，左右移动鼠标
			}
			
		}
		break;
	}
	case WM_LBUTTONUP:
	{
		bLBDown = false;	//鼠标左键弹起
		break;
	}
	case WM_RBUTTONDOWN:
	{
		tagPOINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWnd, &pt);    // 转屏幕座标,如果用屏幕扩展就会出错

		if (g_plot.IsinRect(pt))
		{
			g_plot.Set_m_bShowCoord(true);
			g_plot.Set_tagPt(pt);
		}
			
		break;
	}
	case WM_RBUTTONUP:
	{
		g_plot.Set_m_bShowCoord(false);
		break;
	}
	case WM_SIZE:
		g_plot.OnReshape();
		break;
	case WM_MOUSEWHEEL:
		if ((INT)wParam > 0)
			g_plot.OnScale(1.25);
		else
			g_plot.OnScale(0.8);
		break;
	/************************************************************************/


	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择: 
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO:  在此添加任意绘图代码...
		g_plot.Draw();
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		g_plot.DestroyPlot();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
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


