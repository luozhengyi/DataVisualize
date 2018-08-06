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



//����ȫ�ֱ���
HINSTANCE hInst;						//ȫ��ʵ�����
TCHAR szTitle[MAX_LOADSTRING];			//����������
TCHAR szWindowClass[MAX_LOADSTRING];	//����������
HWND g_MainWnd = NULL;					//�����ھ��

CWndManager g_WndManager;				//���ڹ�����
TCHAR szChildClass[MAX_LOADSTRING];		//�Ӵ�������
HWND g_hMDIClientWnd = NULL;			//�ͻ������ھ��
RECT g_ClientRect;						//�ͻ�����
MYRECT g_ClientMyRect;					//�ͻ�����
#define  TOOLS_HIGH 40					//�����Ӵ��ڵ�ռ�ձ�
int ErrorCode;

////
HWND g_Edit_FrameNum = nullptr;			//���ݲɼ���FrameNum�ı�����

// GDI+��ʼ����Ҫ�ı��������Լ��ı���
Gdiplus::GdiplusStartupInput g_Gdiplus = NULL;
ULONG_PTR g_GdiToken = NULL;

//  ��ͼ�����Զ������
tagPOINT pt1 = { 0, 0 };
tagPOINT pt2 = { 0, 0 };
BOOL bLBDown = false;

//��������
ATOM MyRegisterClass(HINSTANCE);								//ע������������Ӵ�����
LRESULT CALLBACK MDIFrameWndProc(HWND, UINT, WPARAM, LPARAM);	//�����ڵĹ��̺���
LRESULT CALLBACK MDIChildWndProc(HWND, UINT, WPARAM, LPARAM);	//�Ӵ��ڵĹ��̺���
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); //"����"���ڻص�����
INT_PTR CALLBACK DataAcq(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); //"���ݲɼ�"���ڻص�����
BOOL InitInstance(HINSTANCE, int);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;


	//GDI+��ʼ��
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

	//�ر�GDI+����
	Gdiplus::GdiplusShutdown(g_GdiToken);

	return (int)msg.wParam;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����


	//1������������
	//2�����ڱ���
	//3�����ڵ���ʽ
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
	/****************************************ע����������*****************************************/
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
	//�Լ������Ĳ˵����޸Ĳ˵�IDʱ��ͷ�ļ�����Դ�ļ����涼Ҫ��
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_DATAVISUALIZE_DEMO);;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if (!RegisterClassEx(&wcex))
		return FALSE;

	/****************************************ע���Ӵ�����*****************************************/
	wcex.lpfnWndProc = (WNDPROC)MDIChildWndProc;
	wcex.hIcon = NULL; //LoadIcon(hInst, IDNOTE);
	wcex.lpszMenuName = (LPCTSTR)NULL;
	wcex.cbWndExtra = NULL;// CBWNDEXTRA;
	wcex.lpszClassName = szChildClass;

	return RegisterClassEx(&wcex);
}


/*
* @fn: MDIFrameWndProc
* @Remarks: �����ڹ��̺���
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

		// �������ڵ� WM_CREATE ��Ϣ�д����ͻ�����(�ͻ�������ϵͳ�Զ�ע���)
		MDIClientCreateStruct.hWindowMenu = NULL;
		MDIClientCreateStruct.idFirstChild = 1024;
		g_hMDIClientWnd = CreateWindowEx(NULL,
			TEXT("MDICLIENT"), // ϵͳԤ���������
			NULL,
			WS_CHILD | MDIS_ALLCHILDSTYLES,
			g_ClientRect.left, g_ClientRect.top,
			g_ClientRect.right - g_ClientRect.left,
			g_ClientRect.bottom - g_ClientRect.top,
			hWnd, (HMENU)0xCAC, hInst, (void*)&MDIClientCreateStruct);


		g_WndManager.Init(g_hMDIClientWnd);


		RECT newChildRect = { g_ClientRect.left, g_ClientRect.top, g_ClientRect.right, g_ClientRect.top + TOOLS_HIGH };
		g_WndManager.AddChildWnd(E_WND_Tool, newChildRect, ToolWndProc);	//�����������Ӵ���


		newChildRect = { g_ClientRect.left, g_ClientRect.top + TOOLS_HIGH, g_ClientRect.right/2, g_ClientRect.bottom };
		g_WndManager.AddChildWnd(E_WND_Plot_Left, newChildRect, PlotWndProc);	//������߻�ͼ�Ӵ���

		newChildRect = { g_ClientRect.left+g_ClientRect.right / 2, g_ClientRect.top + TOOLS_HIGH, g_ClientRect.right, g_ClientRect.bottom };
		g_WndManager.AddChildWnd(E_WND_Plot_Right, newChildRect, PlotWndProc);	//�����ұ߻�ͼ�Ӵ���

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
			memset(&stOpenFile, 0, sizeof(OPENFILENAME));	//�����ʼ���ᵯ���Ի���ʧ��
			//wchar_t wcFilePath[MAX_PATH] = _T("F:\\Program\\���ƽ���\\��\\MapEditor\\Debug");
			wchar_t wcFilePath[MAX_PATH] = { _T("") };
			stOpenFile.lpstrInitialDir = _T(".\\");
			stOpenFile.lpstrFile = wcFilePath;
			stOpenFile.nMaxFile = MAX_PATH;
			stOpenFile.lpstrFilter = L"*.bin\0\0";		//����Ҫ������NULL��β
			stOpenFile.lpstrDefExt = L"bin";
			stOpenFile.lStructSize = sizeof(OPENFILENAME);
			stOpenFile.hwndOwner = hWnd;
			//stOpenFile.Flags = stOpenFile.Flags | OFN_ENABLEHOOK;
			//stOpenFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
			//stOpenFile.lpfnHook = (LPOFNHOOKPROC)DataAcq;
			if (GetOpenFileName(&stOpenFile))	//�����Ի���
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
			memset(&stSaveFile, 0, sizeof(OPENFILENAME));	//�����ʼ���ᵯ���Ի���ʧ��
			//wchar_t wcFilePath[MAX_PATH] = _T("F:\\Program\\���ƽ���\\��\\MapEditor\\Debug");
			wchar_t wcFilePath[MAX_PATH] = { _T("δ����") };
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
			if (GetSaveFileName(&stSaveFile))	//�����Ի���
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
			DefFrameProc(hWnd, g_hMDIClientWnd, message, wParam, lParam);	//����δ�������Ϣ
			break;
		}


		break;
	}
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO:  �ڴ���������ͼ����...
		EndPaint(hWnd, &ps);
		break;


	case WM_ACTIVATE:	//����ʧȥ���߻�ý���
	{
		//֪ͨ�Ӵ���ʧȥ���߻�ý���
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

// �����ݲɼ��������Ϣ�������
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
		//���ĵ����ĵ�����������200�����أ����ϵ��������Ȳ�������
		MoveWindow(hDlg, 0, 0, 800, 200, true);	
		//Ĭ�ϵĴ��ڴ�С�ǣ�758*576�����ϸı��Ϊ��758*776
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
		// �����˵�ѡ��: 
		switch (wmId)
		{
		case 0x350:		//FrameNum��Ӧ���ı���
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