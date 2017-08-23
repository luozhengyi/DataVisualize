// DataVisualize_Demo.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "DataVisualize_Demo.h"
#include "Plot.h"
#include <GL/gl.h>  
#include <GL/glu.h>  
#include <GL/glaux.h>  

#define MAX_LOADSTRING 100


// ȫ�ֱ���: 
HINSTANCE hInst;								// ��ǰʵ��
TCHAR szTitle[MAX_LOADSTRING];					// �������ı�
TCHAR szWindowClass[MAX_LOADSTRING];			// ����������
/*  �Զ������  */
CPlot g_plot;
tagPOINT pt1 = { 0, 0 };
tagPOINT pt2 = { 0, 0 };
bool bLBDown = false;


// �˴���ģ���а����ĺ�����ǰ������: 
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

 	// TODO:  �ڴ˷��ô��롣
	MSG msg;
	HACCEL hAccelTable;

	// ��ʼ��ȫ���ַ���
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_DATAVISUALIZE_DEMO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ִ��Ӧ�ó����ʼ��: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DATAVISUALIZE_DEMO));

	// ����Ϣѭ��: 
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

   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

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
	/************************************�Լ�����Ϣ*****************************/
	case WM_CREATE:
	{
		g_plot.Init(hWnd);						//��ʼ��OpenGL��ͼ����
		break;
	}
	case WM_LBUTTONDOWN:
	{
		GetCursorPos(&pt1);
		ScreenToClient(hWnd, &pt1);    // ת��Ļ����,�������Ļ��չ�ͻ����

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
			ScreenToClient(hWnd, &pt2);		//�������Ļ��չ�ͻ����

			if (g_plot.IsinRect(pt1))
			{
				g_plot.OnTranslate(pt1, pt2);
				pt1 = pt2;	//�ܹؼ������һֱ���Ų��ţ������ƶ����
			}
			
		}
		break;
	}
	case WM_LBUTTONUP:
	{
		bLBDown = false;	//����������
		break;
	}
	case WM_RBUTTONDOWN:
	{
		tagPOINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWnd, &pt);    // ת��Ļ����,�������Ļ��չ�ͻ����

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
		// �����˵�ѡ��: 
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
		// TODO:  �ڴ���������ͼ����...
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

// �����ڡ������Ϣ�������
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


