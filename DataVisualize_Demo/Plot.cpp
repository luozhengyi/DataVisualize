#include "stdafx.h"
#include "Plot.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <atlstr.h>
#include<algorithm>
#include <math.h>
#include <valarray>
#include <windows.h>
#include <ctime>
#include <mutex>
#include "WndManager.h"
#include "DSP.h"
/***************************OpenGL头文件/静态库****************************/
#include <GL/gl.h>  
#include <GL/glu.h>  
#include <GL/glaux.h>  
#pragma comment(lib,"gl//glu32.lib")
#pragma comment(lib,"gl//glaux.lib")
#pragma comment(lib,"gl//opengl32.lib")
/************************************************************************/

#pragma comment(lib,"ws2_32.lib")


//静态变量定义
int CPlot::objNum = 0;	//类实例化个数

enDataSource CPlot::m_enDataSource = en_From_File;								//数据来源
wchar_t*	 CPlot::m_szDataPath = new wchar_t[NAME_LEN]();						//数据文件路径
int			 CPlot::m_iDataLen = 0;												//数据长度
INT16*		 CPlot::m_pDataX = new INT16[g_sockFrameLen / 2]();					//数据向量
INT16*		 CPlot::m_pDataY = new INT16[g_sockFrameLen / 2]();					//数据向量
enDrawStatus CPlot::m_enDrawStatus = E_Draw_Fresh;								//E_Draw_Fresh：表示绘制实时数据；
stFile		 CPlot::m_stFile;													//数据采集信息结构体


float			CPlot::m_fs = 20;				//采样频率：单位MHZ
double*			CPlot::m_fft_pin = NULL;		//fft输入信号
double*			CPlot::m_fft_pabs = NULL;		//fft输出信号的绝对值
fftw_complex*	CPlot::m_fft_pout = NULL;		//fft输出复信号



/************************************************************************/
/* TCP接收回调函数                                                  */
/************************************************************************/
//const int g_sockFrameLen = sizeof(short) * 21413580;
const int g_sockFrameLen = sizeof(short) * 262144;
//const int g_sockFrameLen = sizeof(short) * 524288;
char ato_buff[g_sockFrameLen];	//一帧长度
std::mutex g_mtx;
extern CWndManager g_WndManager;

void CallBack_RecvProc(void* p)
{

	do
	{
		SOCKET sockClient = *(SOCKET*)p;
		if (sockClient == INVALID_SOCKET)
			break;

		int recvcount = 0;
		int irecv = 0, i = 0;
		//CMyMutex m_mtx;

		while (1)
		{
			//接收数据
			if (recvcount == 0)
				g_mtx.lock();
			irecv = recv(sockClient, (char*)ato_buff + recvcount, g_sockFrameLen - recvcount, 0);
			if (irecv == 0 || irecv == SOCKET_ERROR)
			{
				*(SOCKET*)p = INVALID_SOCKET;
				if (recvcount >= 0)	//数据未收满一帧，tcp连接中断
					g_mtx.unlock();
				_endthread();//删除线程
				return;
			}


			recvcount = (recvcount + irecv) % g_sockFrameLen;
			if (recvcount == 0)
			{
				do 
				{
					if (CPlot::GetstFile().bfile && CPlot::GetstFile().frameNum >= 0)
					{
						if (CPlot::GetstFile().frameNum == 0)
						{
							CPlot::GetstFile().StopRead();
							CPlot::SetDrawStatus(E_Draw_Fresh);
							break;
						}	
						fwrite(ato_buff, g_sockFrameLen, 1, CPlot::GetstFile().fp);
						CPlot::GetstFile().frameNum--;
					}
				} while (0);
				g_mtx.unlock();
			}
				
		}

	} while (false);
}




CPlot::CPlot()
{
	objNum++;
	m_enFigureType = E_FigureType_Normal;	//绘制普通的图
	m_enDrawType = en_Draw_Orignal;			//绘制原始信号图

	m_bShowCoord = false;
	m_hOldFont = NULL;
	m_hNewFont = NULL;

	m_dsp = new CDSP(this);

}

CPlot::~CPlot()
{
	objNum--;
	if (objNum == 0)
	{
		//清除数据向量
		if (m_szDataPath)
			delete[] m_szDataPath;
		if (m_pDataX != NULL)
			delete[] m_pDataX;
		if (m_pDataY != NULL)
			delete[] m_pDataY;
		if (m_fft_pin)
			fftw_free(m_fft_pin);
		if (m_fft_pout)
			fftw_free(m_fft_pout);
		if (m_fft_pabs)
			fftw_free(m_fft_pabs);
		m_fft_pin = NULL;
		m_fft_pout = NULL;
		m_fft_pabs = NULL;
	}	

	//还原绘图环境
	DestroyPlot();

	//DSP资源释放
	if (m_dsp != nullptr)
		delete m_dsp;

}

unsigned CPlot::OnTimer(unsigned timerID, int iParam, string strParam)
{
	unsigned uiRet = 1;
	switch (timerID)
	{
	case DRAW_TIMER:
	{
		if (m_enDrawStatus == E_Draw_Fresh)
		{
			g_mtx.lock();
			GetData();
			g_mtx.unlock();

		}
		
		//遍历所有子窗口
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			if (it->second == PlotWndProc && it->first->GetWndStatus() == E_WND_Show)	//通知绘图窗口进行绘图
				SendMessage(it->first->GetWnd(), WM_PAINT, NULL, NULL);	//在WM_PAINT消息里面就不闪烁了是为啥？
			++it;
		}
		
		break;
	}	
	default:
		break;
	}
	return uiRet;
}

bool CPlot::Init(HWND hWnd)
{
	bool bRet = false;
	do
	{
		if (hWnd == INVALID_HANDLE_VALUE)
			break;
		m_hWnd = hWnd;
		m_hDC = GetDC(m_hWnd);				//得到当前窗口的设备环境  
		//设置OpenGL绘图的矩形区域
		SetFigRect();
		//设置win32中的OpenGL环境
		SetupPixelFormat();					//调用像素格式设置函数  
		m_hRC = wglCreateContext(m_hDC);	//创建OpenGL绘图环境并创建一个指向OpenGL绘制环境的句柄  
		wglMakeCurrent(m_hDC, m_hRC);		//将传递过来的绘制环境设置为OpenGL将要进行绘制的当前绘制环境  
		//初始化OpenGL绘图环境
		InitGL();
		InitText();



		if (m_enDataSource != en_From_Sock)
		{
			GetData();
			GetMaxMinValue();
		}

		//只需要让一个绘图窗口创建定时器就行。
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			CPlot *pPlot = it->first->GetPlotObj();
			CPlot *pPlot1 = this;
			if ((it->first->GetWnd()) == hWnd)
			{
				if (it->first->GetWndID() == E_WND_Plot_Right)
				{
					ClearTimer();	//先清理所有定时器
					AddTimer(DRAW_TIMER, 100);
					//AddTimer(GETDATA_TIMER, 50);
				}
			}
			
			++it;
		}

		bRet = true;
	} while (false);

	return bRet;
}

void CPlot::InitGL()
{
	glShadeModel(GL_SMOOTH);                 // 启用阴影平滑  
	// 设置窗体背景颜色
	glClearColor(m_stFigClrInfo.ClrWnd.R, m_stFigClrInfo.ClrWnd.G, m_stFigClrInfo.ClrWnd.B, m_stFigClrInfo.ClrWnd.A);
	glClearDepth(1.0f);                      // 设置深度缓存   
	glDepthFunc(GL_LEQUAL);                  // 所作深度测试的类型  
	glEnable(GL_DEPTH_TEST);                 // 启用深度测试 
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);// 告诉系统对透视进行修正

}

void CPlot::InitText()
{
	//创建字体
	m_hNewFont = CreateFontW(
		18,	//字体的高度
		0,	//字体的宽度
		0,	//角度
		0,	//角度
		0,	//磅数
		0,	//是否斜体
		0,	//非下划线
		0,	//删除线
		GB2312_CHARSET,	//字符集,这个参数比较重要
		0,	//精度
		0,	//精度
		0,	//质量
		0,
		L"微软雅黑"	//字体，很重要
		);
	m_hOldFont = SelectObject(m_hDC, m_hNewFont);
}

void CPlot::SetupPixelFormat() //为设备环境设置像素格式  
{
	int nPixelFormat; //像素格式变量  
	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), //数据结构大小  
		1,							//版本号，总设为1  
		PFD_DRAW_TO_WINDOW |			//支持窗口  
		PFD_SUPPORT_OPENGL |			//支持OpenGL  
		PFD_DOUBLEBUFFER,			//支持双缓存  
		PFD_TYPE_RGBA,				//RGBA颜色模式  
		32,							//32位颜色模式  
		0, 0, 0, 0, 0, 0,			//忽略颜色为，不使用  
		0,				//无alpha缓存  
		0,				//忽略偏移位  
		0,				//无累积缓存  
		0, 0, 0, 0,		//忽略累积位  
		16,				//16位z-buffer（z缓存）大小  
		0,				//无模板缓存  
		0,				//无辅助缓存  
		PFD_MAIN_PLANE, //主绘制平面  
		0,				//保留的数据项  
		0, 0, 0 };		//忽略层面掩模  
	//选择最匹配的像素格式，返回索引值  
	nPixelFormat = ChoosePixelFormat(m_hDC, &pfd);
	//设置环境设备的像素格式  
	bool bRet = SetPixelFormat(m_hDC, nPixelFormat, &pfd);
}

void CPlot::Draw()
{
	wglMakeCurrent(m_hDC, m_hRC);	// MDI 在多个子窗口之间来回调换绘制时要加的一句话，不然只会有一个子窗口能正常绘制，其它的都变黑了


	// 设置窗体背景颜色
	glClearColor(m_stFigClrInfo.ClrWnd.R, m_stFigClrInfo.ClrWnd.G, m_stFigClrInfo.ClrWnd.B, m_stFigClrInfo.ClrWnd.A);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清屏和清除深度缓冲区  

	//进行视角变换
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//设置可见坐标区域
	gluOrtho2D(m_stAxisInfo.xmin, m_stAxisInfo.xmax, m_stAxisInfo.ymin, m_stAxisInfo.ymax);
	//设置投影到屏幕上的区域
	glViewport(m_FigRect.x, m_FigRect.y, m_FigRect.width, m_FigRect.heigh);


	//绘制图形区的背景
	DrawBG();



	//画坐标
	DrawAxis();

	//g_mtx.lock();
	//绘制数据
	DrawData();
	//g_mtx.unlock();

	glFlush();
	bool bRet = SwapBuffers(m_hDC);

	//利用GDI绘制坐标刻度，必须在最后画，不然会被OpenGL覆盖
	DrawText();
	//定标
	if (m_bShowCoord)
		ShowPosCoord(m_tagPt);



}

void CPlot::DrawAxis()
{
	glColor3f(m_stFigClrInfo.ClrAxis.R, m_stFigClrInfo.ClrAxis.G, m_stFigClrInfo.ClrAxis.B);

	//绘制网格
	glLineStipple(1, 0x0F0F);	//设置虚线模式
	glEnable(GL_LINE_STIPPLE);	//启动虚线模式
	glLineWidth(1.0f);			//限制线宽度
	glBegin(GL_LINES);

	m_stAxisInfo.xscale = (m_stAxisInfo.xmax - m_stAxisInfo.xmin) / 10.0;
	m_stAxisInfo.yscale = (m_stAxisInfo.ymax - m_stAxisInfo.ymin) / 10.0;

	//y轴方向网格
	float y = m_stAxisInfo.ymin;
	for (; y <= m_stAxisInfo.ymax; y += m_stAxisInfo.yscale)
	{
		glVertex2f(m_stAxisInfo.xmin, y);
		glVertex2f(m_stAxisInfo.xmax, y);
	}
	//x轴方向网格
	float x = m_stAxisInfo.xmin;
	for (; x <= m_stAxisInfo.xmax; x += m_stAxisInfo.xscale)
	{
		glVertex2f(x, m_stAxisInfo.ymin);
		glVertex2f(x, m_stAxisInfo.ymax);
	}
	glEnd();
	glDisable(GL_LINE_STIPPLE);	//关闭虚线模式


	//绘制边框
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	//x轴方向边框
	glVertex2f(m_stAxisInfo.xmin, m_stAxisInfo.ymin);
	glVertex2f(m_stAxisInfo.xmax, m_stAxisInfo.ymin);
	glVertex2f(m_stAxisInfo.xmin, m_stAxisInfo.ymax);
	glVertex2f(m_stAxisInfo.xmax, m_stAxisInfo.ymax);
	//y轴方向边框
	glVertex2f(m_stAxisInfo.xmin, m_stAxisInfo.ymin);
	glVertex2f(m_stAxisInfo.xmin, m_stAxisInfo.ymax);
	glVertex2f(m_stAxisInfo.xmax, m_stAxisInfo.ymin);
	glVertex2f(m_stAxisInfo.xmax, m_stAxisInfo.ymax);
	glEnd();
}

void CPlot::DrawBG()
{
	//绘制图形区的背景
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清屏和清除深度缓冲区  
	glColor3f(m_stFigClrInfo.ClrBG.R, m_stFigClrInfo.ClrBG.G, m_stFigClrInfo.ClrBG.B);
	glRectf(m_stAxisInfo.xmin, m_stAxisInfo.ymin, m_stAxisInfo.xmax, m_stAxisInfo.ymax);
}

void CPlot::DrawData()
{
	switch (m_enDrawType)
	{
	case en_Draw_Orignal:
		DrawOrignal();
		break;
	case en_Draw_Spectrum:
		DrawSpectrum();
		break;
	case en_Draw_Constellation:
		DrawConstellation();
		break;
	default:
		break;
	}
}

void CPlot::DrawText()
{
	char szCoord[100];	//坐标刻度
	memset(szCoord, 0, 100);

	//计算字符串占的宽高像素。
	SIZE szText;


	//RECT rect = { 10, 10, 50, 26 };	//默认字体将近一个字母：宽为8个像素，高为16个像素
	//::DrawTextA(m_hDC, "hello", 5,&rect, 0);

	//绘制坐标刻度
	float fTemp = 0.0;
	int xPos = 0;
	int yPos = 0;

	fTemp = m_stAxisInfo.xmin;
	for (int i = 0; fTemp <= m_stAxisInfo.xmax + m_stAxisInfo.xscale/100.0; fTemp += m_stAxisInfo.xscale, ++i)
	{
		//stAxisInfo.xmin
		memset(szCoord, 0, 100);
		sprintf_s(szCoord, "%.3f", fTemp);	//将数字(保留小数点后三位)转化为字符串，
		fTemp = 0.0;
		sscanf_s(szCoord, "%f", &fTemp);	//字符串转化为数字
		sprintf_s(szCoord, "%G", fTemp);	//将数字转化为字符串，并舍去小数末尾的0
		GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);	//计算字符串所占用的像素
		xPos = m_FigRect.x +i*m_FigRect.width/10.0;
		yPos = m_iClientHeight - m_FigRect.y;
		TextOutA(m_hDC, xPos - szText.cx / 2, yPos + 3, szCoord, strlen(szCoord));
	}
	
	fTemp = m_stAxisInfo.ymin;
	for (int i = 0; fTemp <= m_stAxisInfo.ymax + m_stAxisInfo.yscale / 100.0; fTemp += m_stAxisInfo.yscale, ++i)
	{
		memset(szCoord, 0, 100);
		sprintf_s(szCoord, "%.3f", fTemp);	//数子转化为字符串
		fTemp = 0.0;
		sscanf_s(szCoord, "%f", &fTemp);	//字符串转化为数字
		sprintf_s(szCoord, "%G", fTemp);	//将数字转化为字符串，并舍去小数末尾的0
		GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);
		xPos = m_FigRect.x;
		yPos = m_iClientHeight - m_FigRect.y - i*m_FigRect.heigh / 10.0;
		TextOutA(m_hDC, xPos - szText.cx - 4, yPos - szText.cy / 2, szCoord, strlen(szCoord));
	}

}

void CPlot::SetFigRect()
{
	//窗口客户区
	GetClientRect(m_hWnd, &m_ClientRect);
	m_iClientWidth = m_ClientRect.right - m_ClientRect.left;
	m_iClientHeight = m_ClientRect.bottom - m_ClientRect.top;
	//绘图区信息
	m_FigRect.x = m_iClientWidth * 0.05;
	m_FigRect.y = m_iClientHeight * 0.05;
	m_FigRect.width = m_iClientWidth*0.9;
	m_FigRect.heigh = m_iClientHeight*0.9;
}


void CPlot::GetData()
{
	switch (m_enDataSource)
	{
	case en_From_File:
		//GetXYDataFromFile();
		GetYDataFromFile();
		break;
	case en_From_Sock:
		GetDataFromSock();
		break;
	default:
		break;
	}
}

bool CPlot::GetYDataFromFile()	//获取绘图数据
{
	//GetDataPath();
	bool bRet = false;

	do
	{
		if (wcslen(m_szDataPath) == 0)
			break;
		fstream file1;
		file1.open(m_szDataPath, ios::in | ios::binary);
		if (!file1)
			break;
		//读取文件，写如文件，按行处理
		int num = 0;
		char ch[2];
		short sData = 0;
		memset(m_pDataX, 0, g_sockFrameLen);
		memset(m_pDataY, 0, g_sockFrameLen);
		while (!file1.eof())
		{
			memset(ch, 0, sizeof(ch));
			sData = 0;
			file1.read(ch, sizeof(ch));	//读取两个字节
			if (file1.good())			//防止到文件结束处多读取一次
			{
				memcpy(&sData, ch, 2);
				m_pDataY[num] = sData;
				m_pDataX[num] = num;
				num++;
				if (num == g_sockFrameLen / 2)
					break;
			}
		}
		file1.close();
		m_iDataLen = num / 2 * 2;	//获取数据长度,fft变换的数据长度必须为2的倍数，奇数个会出错！
		fft();
		bRet = true;
	} while (false);
	return bRet;

}

bool CPlot::GetXYDataFromFile()
{
	//GetDataPath();
	bool bRet = false;

	do
	{
		if (wcslen(m_szDataPath) == 0)
			break;
		fstream file1;
		file1.open(m_szDataPath, ios::in | ios::binary);
		if (!file1)
			break;
		//读取文件，写如文件，按行处理
		int num = 0;
		char ch[4];
		short IData = 0;
		short QData = 0;
		memset(m_pDataX, 0, g_sockFrameLen);
		memset(m_pDataY, 0, g_sockFrameLen);
		while (!file1.eof())
		{
			memset(ch, 0, sizeof(ch));
			IData = 0;
			QData = 0;
			file1.read(ch, sizeof(ch));	//读取4个字节
			if (file1.good())			//防止到文件结束处多读取一次
			{
				memcpy(&IData, ch, 2);
				memcpy(&QData, ch + 2, 2);
				m_pDataY[num] = IData;
				m_pDataX[num] = QData;
				num++;
				if (num == g_sockFrameLen / 2)
					break;
			}
		}
		file1.close();
		m_iDataLen = num / 2 * 2;	//获取数据长度,fft变换的数据长度必须为2的倍数，奇数个会出错！
		fft();
		bRet = true;
	} while (false);
	return bRet;

}

bool CPlot::GetDataFromSock()
{
	bool bRet = false;
	do
	{
		GetSock();
		if (m_stSockInfo.sockClient == INVALID_SOCKET)
			break;

		int num = 0;
		short* pbuff = (short*)ato_buff;

		short tempYMaxVal = -32768;
		short tempYMinVal = 32767;

		memset(m_pDataX, 0, g_sockFrameLen);
		memset(m_pDataY, 0, g_sockFrameLen);
		for (; num < g_sockFrameLen / 2; num++)
		{
			m_pDataY[num] = pbuff[num];
			m_pDataX[num] = num;

			if (m_pDataY[num] > tempYMaxVal)
				tempYMaxVal = m_pDataY[num];
			if (m_pDataY[num] < tempYMinVal)
				tempYMinVal = m_pDataY[num];


		}
		if (tempYMinVal == 0 && tempYMaxVal == 0)
			m_iDataLen = 0;
		else
			m_iDataLen = num;
		fft();

		bRet = true;
	} while (false);
	return bRet;

}

void CPlot::GetMaxMinValue()	//获取数据的最大值或最小值
{
	if (m_iDataLen == 0 || (m_fft_pout == NULL))
		return;
	
	if (m_enDrawStatus == E_Draw_Static)
		return;


	//int iLen = m_iDataLen / 4;
	int iLen = 65536;
	switch (m_enDrawType)
	{
	case en_Draw_Orignal:
	{


		//y轴
		short tempYMaxVal = -32768;
		short tempYMinVal = 32767;
		for (int i = 0; i < iLen; i++)
		{
			if (m_pDataY[i] > tempYMaxVal)
				tempYMaxVal = m_pDataY[i];
			if (m_pDataY[i] < tempYMinVal)
				tempYMinVal = m_pDataY[i];
		}

		if (tempYMinVal == 0 && tempYMaxVal == 0)		//刚开始时，socket连接好了，但是并没有传数据过来
		{
			m_stAxisInfo.ymin = 0;
			m_stAxisInfo.ymax = 1;
		}
		//如果 stAxisInfo.ymin = stAxisInfo.ymax = 0  会导致程序卡死
		else
		{
			m_stAxisInfo.ymin = tempYMinVal;
			m_stAxisInfo.ymax = tempYMaxVal;
		}

		//x轴
		m_stAxisInfo.xmin = 0;
		m_stAxisInfo.xmax = iLen;
		break;
	}
	case en_Draw_Spectrum:
	{
		//y轴
		m_stAxisInfo.ymin = -140;
		m_stAxisInfo.ymax = 0;


		//x轴
		m_stAxisInfo.xmin = 0;
		m_stAxisInfo.xmax = m_fs / 2;
		break;
	}

	case en_Draw_Constellation:
	{
		//y轴
		//std::vector<double>::iterator it_y = m_veDataY.begin();
		//stAxisInfo.ymin = *(std::min_element(it_y, m_veDataY.end()));
		//stAxisInfo.ymax = *(std::max_element(it_y, m_veDataY.end()));
		m_stAxisInfo.ymin = -1.2;
		m_stAxisInfo.ymax = 1.2;

		////x轴
		//std::vector<double>::iterator it_x = m_veDataX.begin();
		//stAxisInfo.xmin = *(std::min_element(it_x, m_veDataX.end()));

		//stAxisInfo.xmax = *(std::max_element(it_x, m_veDataX.end()));
		m_stAxisInfo.xmin = -1.2;
		m_stAxisInfo.xmax = 1.2;
		break;
	}
	default:
		break;
	}

}

void CPlot::GetSock()
{
	do
	{
		if (m_stSockInfo.sockClient != INVALID_SOCKET)
			break;

		//先清理Socket，可以断开后再自动连接
		closesocket(m_stSockInfo.sockSrv);
		WSACleanup();
			

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			::MessageBoxA(NULL, "WSA Startup Failed!", "Error", NULL);
			break;
		}

		//创建用于监听的套接字,即服务端的套接字
		m_stSockInfo.sockSrv = socket(AF_INET, SOCK_STREAM, 0);

		SOCKADDR_IN addrSrv;

		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(m_stSockInfo.sockPort); //1024以上的端口号
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

		if (SOCKET_ERROR == ::bind(m_stSockInfo.sockSrv, (LPSOCKADDR)&addrSrv, sizeof(SOCKADDR_IN)))
		{
			printf("Failed bind:%d\n", WSAGetLastError());
		}

		if (listen(m_stSockInfo.sockSrv, 10) == SOCKET_ERROR){
			printf("Listen failed:%d", WSAGetLastError());
		}

		SOCKADDR_IN addrClient;
		int len = sizeof(SOCKADDR);

		//等待客户请求到来,会将 定时器线程 阻塞
		m_stSockInfo.sockClient = accept(m_stSockInfo.sockSrv, (SOCKADDR *)&addrClient, &len);
		if (m_stSockInfo.sockClient == INVALID_SOCKET)
		{
			//::MessageBoxA(NULL, "Accept Failed!", "Error", NULL);
			break;
		}

		//创建接收线程
		_beginthread(CallBack_RecvProc, 0, &m_stSockInfo.sockClient); //创建定时器线程以用来运行定时器

		//printf("Accept client IP:[%s]\n", inet_ntoa(addrClient.sin_addr));

	} while (0);

}

void CPlot::GetDataPath(wchar_t* wszPath)
{
	wmemcpy(m_szDataPath, wszPath, wcslen(wszPath));
	//wmemcpy(m_szDataPath, "bbb_transfer1.bin", strlen("bbb_transfer1.bin"));
	//memcpy(m_szDataPath, "ccc.bin", strlen("ccc.bin"));
}

void CPlot::OnReshape()
{
	//m_hDC = GetDC(m_hWnd);				//得到当前窗口的设备环境  
	GetClientRect(m_hWnd, &m_ClientRect);
	m_iClientWidth = m_ClientRect.right - m_ClientRect.left;
	m_iClientHeight = m_ClientRect.bottom - m_ClientRect.top;
	SetFigRect();
	wglDeleteContext(m_hRC);
	m_hRC = wglCreateContext(m_hDC);	//创建OpenGL绘图环境并创建一个指向OpenGL绘制环境的句柄  
	wglMakeCurrent(m_hDC, m_hRC);		//将传递过来的绘制环境设置为OpenGL将要进行绘制的当前绘制环境  

}

void CPlot::OnTranslate(const tagPOINT ptPre, const tagPOINT ptNow)	//平移图像,图像随着鼠标移动
{
	//平移x轴
	float fxOffSent = ScreentoOpenGLCoord(ptPre).x - ScreentoOpenGLCoord(ptNow).x;
	m_stAxisInfo.xmin += fxOffSent;
	m_stAxisInfo.xmax += fxOffSent;
	//平移y轴
	float fyOffSent = ScreentoOpenGLCoord(ptPre).y - ScreentoOpenGLCoord(ptNow).y;
	m_stAxisInfo.ymin += fyOffSent;
	m_stAxisInfo.ymax += fyOffSent;

}

void CPlot::OnScale(float fTime)
{//缩放图像,不让它一直缩小，但可以一直放大
	/*if (m_iScaleCount >= 1)
	{
	if (fTime > 1)
	m_iScaleCount++;
	else
	m_iScaleCount--;
	//缩放x轴
	stAxisInfo.xmin = stAxisInfo.xmin / fTime;
	stAxisInfo.xmax = stAxisInfo.xmax / fTime;
	//缩放y轴
	/ *stAxisInfo.ymin = stAxisInfo.ymin / fTime;
	stAxisInfo.ymax = stAxisInfo.ymax / fTime;* /
	}
	else if (m_iScaleCount == 0)
	{
	if (fTime > 1)
	{
	m_iScaleCount++;
	//缩放x轴
	stAxisInfo.xmin = stAxisInfo.xmin / fTime;
	stAxisInfo.xmax = stAxisInfo.xmax / fTime;
	//缩放y轴
	/ *stAxisInfo.ymin = stAxisInfo.ymin / fTime;
	stAxisInfo.ymax = stAxisInfo.ymax / fTime;* /
	}
	}*/
	/*
	//缩放x轴
	stAxisInfo.xmin = stAxisInfo.xmin / fTime;
	stAxisInfo.xmax = stAxisInfo.xmax / fTime;
	//缩放y轴
	stAxisInfo.ymin = stAxisInfo.ymin / fTime;
	stAxisInfo.ymax = stAxisInfo.ymax / fTime;
	*/

	//保持屏幕左下角不变的
	m_stAxisInfo.xmax = m_stAxisInfo.xmax - (m_stAxisInfo.xmin - m_stAxisInfo.xmin / fTime);
	//stAxisInfo.ymax = stAxisInfo.ymax - (stAxisInfo.ymin - stAxisInfo.ymin / fTime);
}

bool CPlot::IsinRect(const tagPOINT pt)	//判断鼠标是否在绘图区域内
{
	if (pt.x<m_FigRect.x ||
		pt.x>m_FigRect.x + m_FigRect.width ||
		pt.y<m_iClientHeight - m_FigRect.y - m_FigRect.heigh ||
		pt.y>m_iClientHeight - m_FigRect.y)
		return false;
	return true;
}

void CPlot::ShowPosCoord(const tagPOINT pt)	//显示点的坐标值
{
	GLPoint Coord = ScreentoOpenGLCoord(pt);
	float fTemp1 = 0.0;
	float fTemp2 = 0.0;

	char szCoord[40];
	memset(szCoord, 0, sizeof(szCoord));
	sprintf_s(szCoord, "%.2f,%.2f", Coord.x, Coord.y);

	sscanf_s(szCoord, "%f,%f", &fTemp1, &fTemp2);	//字符串转化为数字
	sprintf_s(szCoord, "(%G,%G)", fTemp1, fTemp2);		//将数字转化为字符串，并舍去小数末尾的0


	TextOutA(m_hDC, pt.x, pt.y, szCoord, strlen(szCoord));

}

GLPoint CPlot::ScreentoOpenGLCoord(const tagPOINT Screen_pt)
{
	//调用该函数之前应该判断一下该点是否在绘图区
	GLPoint OpenGL_pt = { 0, 0 };
	OpenGL_pt.x = m_stAxisInfo.xmin + (Screen_pt.x - m_FigRect.x) / m_FigRect.width * (m_stAxisInfo.xmax - m_stAxisInfo.xmin);
	OpenGL_pt.y = m_stAxisInfo.ymin + ((m_iClientHeight - m_FigRect.y) - Screen_pt.y) / m_FigRect.heigh* (m_stAxisInfo.ymax - m_stAxisInfo.ymin);
	return OpenGL_pt;
}

tagPOINT CPlot::OpenGLtoScreenCoord(const GLPoint OpenGL_pt)
{
	//调用该函数之前应该判断一下该点是否在绘图区
	tagPOINT Screen_pt = { 0, 0 };
	Screen_pt.x = m_FigRect.x + (OpenGL_pt.x - m_stAxisInfo.xmin) / (m_stAxisInfo.xmax - m_stAxisInfo.xmin) * m_FigRect.width;
	Screen_pt.y = (m_iClientHeight - m_FigRect.y) - (OpenGL_pt.y - m_stAxisInfo.ymin) / (m_stAxisInfo.ymax - m_stAxisInfo.ymin) * m_FigRect.heigh;
	return Screen_pt;
}

void CPlot::DestroyPlot()
{
	wglMakeCurrent(m_hDC, NULL);	//还原当前DC中的RC为NULL
	if (m_hRC != INVALID_HANDLE_VALUE)
	{
		wglDeleteContext(m_hRC);		//删除RC
	}
	if (m_hNewFont)
		DeleteObject(m_hNewFont);
	if (m_hOldFont)
		SelectObject(m_hDC, m_hOldFont);
	ReleaseDC(m_hWnd, m_hDC);

}


/******************************信号处理部分*****************************/
void CPlot::fft()
{


	//fft之前先释放内存，防止内存泄漏
	if (m_fft_pin)
		fftw_free(m_fft_pin);
	if (m_fft_pout)
		fftw_free(m_fft_pout);
	if (m_fft_pabs)
		fftw_free(m_fft_pabs);
	m_fft_pin = NULL;
	m_fft_pout = NULL;
	m_fft_pabs = NULL;

	//OnTimer线程比RecvProc线程先启动，所以第一次取数据为空
	if (m_iDataLen == 0)
		return;

	fftw_plan p;

	m_fft_pin = (double *)fftw_malloc(sizeof(double) * m_iDataLen);
	m_fft_pabs = (double *)fftw_malloc(sizeof(double) * (m_iDataLen / 2 - 1));
	m_fft_pout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_iDataLen);

	if (!m_fft_pin || !m_fft_pout)
		return;

	//赋值
	for (int i = 0; i < m_iDataLen; i++)
	{
		m_fft_pin[i] = ((double)m_pDataY[i] / (1 << 15)) / (m_iDataLen);	//考虑到实际的16bit ADC采集
	}

	// 傅里叶变换
	p = fftw_plan_dft_r2c_1d(m_iDataLen, m_fft_pin, m_fft_pout, FFTW_ESTIMATE);	//实数到复数，由Hermite对称性只取fs的一半

	fftw_execute(p);

	//赋值
	for (int i = 0; i <(m_iDataLen / 2 - 1); i++)
	{
		m_fft_pabs[i] = pow(m_fft_pout[i][0], 2) + pow(m_fft_pout[i][1], 2);
		m_fft_pabs[i] = 10.0 * log10(m_fft_pabs[i]);	//log10(0)会出现莫名其妙的错误
	}


	// 释放资源
	fftw_destroy_plan(p);

}

void CPlot::DrawOrignal()
{
	if (m_iDataLen == 0)
		return;

	int i = 0;
	glDisable(GL_LINE_STIPPLE);
	glLineWidth(2.0);
	glColor3f(m_stFigClrInfo.ClrLine.R, m_stFigClrInfo.ClrLine.G, m_stFigClrInfo.ClrLine.B);
	glBegin(GL_LINE_STRIP);
	while (i < m_iDataLen/4)
	{
		glVertex2f(i, m_pDataY[i]);
		++i;
	}
	glEnd();
}

void CPlot::DrawSpectrum()
{
	if (m_iDataLen == 0 || m_fft_pout == NULL)
		return;

	glDisable(GL_LINE_STIPPLE);
	glLineWidth(2.0);
	glColor3f(m_stFigClrInfo.ClrLine.R, m_stFigClrInfo.ClrLine.G, m_stFigClrInfo.ClrLine.B);

	//float fs = 100;	//100MHZ
	int i = 0;
	glBegin(GL_LINE_STRIP);
	while (i<(m_iDataLen / 2 - 1))
	//while (i<65536)
	{
		glVertex2f(i*m_fs / m_iDataLen, m_fft_pabs[i]);
		++i;
	}
	glEnd();
}

void CPlot::DrawConstellation()
{
	glDisable(GL_LINE_STIPPLE);
	glLineWidth(50.0);
	glColor3f(m_stFigClrInfo.ClrLine.R, m_stFigClrInfo.ClrLine.G, m_stFigClrInfo.ClrLine.B);
	//glColor3f(0.0, 0.0, 1.0);
	//std::vector<double>::iterator itX = m_veDataX.begin();
	//std::vector<double>::iterator itY = m_veDataY.begin();
	glBegin(GL_POINTS);
	//glBegin(GL_LINE_STRIP);
	for (int i = 0; i < m_dsp->m_symbolsLen;i++)
	{
		glVertex2f(m_dsp->m_pISymbols[i], m_dsp->m_pQSymbols[i]);
	}
	glEnd();
}