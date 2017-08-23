#include "stdafx.h"
#include "Plot.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <atlstr.h>
#include<algorithm>
#include <math.h>
#include <valarray>
/***************************OpenGL头文件/静态库****************************/
#include <GL/gl.h>  
#include <GL/glu.h>  
#include <GL/glaux.h>  
#pragma comment(lib,"gl//glu32.lib")
#pragma comment(lib,"gl//glaux.lib")
#pragma comment(lib,"gl//opengl32.lib")
/************************************************************************/
CPlot::CPlot()
{
	//m_ClrBG = { 0.0f, 0.0f, 0.0f, 0.0f };		//绘图区背景黑色
	m_ClrBG = { 0.45f, 0.45f, 0.45f, 0.0f };
	//m_ClrWnd = { 0.51f, 0.51f, 0.51f, 0.0f };	//窗体背景灰色
	m_ClrWnd = { 1.0f, 1.0f, 1.0f, 0.0f };		//窗体背景灰色
	m_ClrLine = { 1.0f, 0.0f, 0.0f, 0.0f };		//绘图红色
	m_ClrText = { 1.0f, 1.0f, 1.0f, 0.0f };		//文字白色
	m_ClrAxis = { 0.0f, 0.0f, 0.0f, 0.0f };		//坐标颜色

	m_enFigureType = E_FigureType_Normal;	//绘制普通的图
	m_enDrawType = en_Draw_Spectrum;	//绘制原始信号图
	m_enDataSource = en_From_File;			//默认从文件读取数据

	m_bShowCoord = false;
	m_hOldFont = NULL;
	m_hNewFont = NULL;
	std::memset(m_szDataPath,0,sizeof(m_szDataPath));	
	m_pData = NULL;
	m_iDataLen = 0;
	m_iScaleCount = 0;

	m_fft_pin = NULL;
	m_fft_pout = NULL;
	m_fs = 100;

	m_xmin = 0.0;
	m_xmax = 1.0;
	m_ymin = 0.0;
	m_ymax = 1.0;
	m_xscale = (m_xmax - m_xmin) / 10.0;
	m_yscale = (m_ymax - m_ymin) / 10.0;

}

CPlot::~CPlot()
{
	//清除数据向量
	if (!m_veDataX.empty())
		m_veDataX.clear();
	if (!m_veDataY.empty())
		m_veDataY.clear();
	fftw_free(m_fft_pin);
	fftw_free(m_fft_pout);
	fftw_free(m_fft_pabs);
}

unsigned CPlot::OnTimer(unsigned timerID, int iParam, string strParam)
{
	switch (timerID)
	{
	case DRAW_TIMER:
		//xaxis += 10;
		//yaxis += 10;
		SendMessage(m_hWnd, WM_PAINT, NULL, NULL);	//在WM_PAINT消息里面就不闪烁了是为啥？
		break;
	default:
		break;
	}
	return 1;
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
		//设置矩形区域
		SetRect();
		//设置win32中的OpenGL环境
		SetupPixelFormat();					//调用像素格式设置函数  
		m_hRC = wglCreateContext(m_hDC);	//创建OpenGL绘图环境并创建一个指向OpenGL绘制环境的句柄  
		wglMakeCurrent(m_hDC, m_hRC);		//将传递过来的绘制环境设置为OpenGL将要进行绘制的当前绘制环境  
		//初始化OpenGL绘图环境
		InitGL();
		InitText();

		GetData();
		GetMaxMinValue();

		AddTimer(DRAW_TIMER, 50);

		bRet = true;
	} while (false);

	return bRet;
}

void CPlot::InitGL()
{
	glShadeModel(GL_SMOOTH);                 // 启用阴影平滑  
	// 设置窗体背景颜色
	glClearColor(m_ClrWnd.R, m_ClrWnd.G, m_ClrWnd.B, m_ClrWnd.A);
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
	static PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), //数据结构大小  
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
	// 设置窗体背景颜色
	glClearColor(m_ClrWnd.R, m_ClrWnd.G, m_ClrWnd.B, m_ClrWnd.A);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清屏和清除深度缓冲区  

	//进行视角变换
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//设置可见坐标区域
	gluOrtho2D(m_xmin, m_xmax, m_ymin , m_ymax);
	//设置投影到屏幕上的区域
	glViewport(m_FigRect.x, m_FigRect.y, m_FigRect.width, m_FigRect.height);

	//绘制图形区的背景
	DrawBG();


	//画坐标
	DrawAxis();

	//绘制数据
	DrawData();

	glFlush();
	SwapBuffers(m_hDC);

	//利用GDI绘制坐标刻度，必须在最后画，不然会被OpenGL覆盖
	DrawText();
	//定标
	if (m_bShowCoord)
		ShowPosCoord(m_tagPt);

	
}

void CPlot::DrawAxis()
{
	glColor3f(m_ClrAxis.R, m_ClrAxis.G, m_ClrAxis.B);

	//绘制网格
	glLineStipple(1, 0x0F0F);	//设置虚线模式
	glEnable(GL_LINE_STIPPLE);	//启动虚线模式
	glLineWidth(1.0f);			//限制线宽度
	glBegin(GL_LINES);

	m_xscale = (m_xmax - m_xmin) / 10.0;
	m_yscale = (m_ymax - m_ymin) / 10.0;

	//x轴方向网格
	float y = m_ymin;
	for (; y <= m_ymax; y+=m_yscale)
	{
		glVertex2f(m_xmin, y);
		glVertex2f(m_xmax, y);
	}
	//y轴方向网格
	float x = m_xmin;
	for (; x<= m_xmax; x+=m_xscale)
	{
		glVertex2f(x, m_ymin);
		glVertex2f(x, m_ymax);
	}
	glEnd();
	glDisable(GL_LINE_STIPPLE);	//关闭虚线模式

	
	//绘制边框
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	//x轴方向边框
	glVertex2f(m_xmin, m_ymin);
	glVertex2f(m_xmax, m_ymin);
	glVertex2f(m_xmin, m_ymax);
	glVertex2f(m_xmax, m_ymax);
	//y轴方向边框
	glVertex2f(m_xmin, m_ymin);
	glVertex2f(m_xmin, m_ymax);
	glVertex2f(m_xmax, m_ymin);
	glVertex2f(m_xmax, m_ymax);
	glEnd();
}

void CPlot::DrawBG()
{
	//绘制图形区的背景
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清屏和清除深度缓冲区  
	glColor3f(m_ClrBG.R, m_ClrBG.G, m_ClrBG.B);
	glRectf(m_xmin, m_ymin, m_xmax, m_ymax);
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
	//m_xmin
	memset(szCoord, 0, 100);
	sprintf_s(szCoord, "%.3f", m_xmin);	//将数字(保留小数点后三位)转化为字符串，
	sscanf_s(szCoord, "%f", &fTemp);	//字符串转化为数字
	sprintf_s(szCoord, "%G", fTemp);	//将数字转化为字符串，并舍去小数末尾的0
	GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);	//计算字符串所占用的像素
	xPos = m_FigRect.x;
	yPos = m_iClientHeight - m_FigRect.y;
	TextOutA(m_hDC, xPos, yPos +3 , szCoord, strlen(szCoord));
	//m_xmax
	memset(szCoord, 0, 100);
	sprintf_s(szCoord, "%.3f", m_xmax);
	sscanf_s(szCoord, "%f", &fTemp);	//字符串转化为数字
	sprintf_s(szCoord, "%G", fTemp);	//将数字转化为字符串，并舍去小数末尾的0
	xPos = m_FigRect.x + m_FigRect.width;
	yPos = m_iClientHeight - m_FigRect.y;
	TextOutA(m_hDC, xPos - szText.cx/2, yPos + 3, szCoord, strlen(szCoord));
	//m_ymin
	memset(szCoord, 0, 100);
	sprintf_s(szCoord, "%.3f", m_ymin);
	sscanf_s(szCoord, "%f", &fTemp);	//字符串转化为数字
	sprintf_s(szCoord, "%G", fTemp);	//将数字转化为字符串，并舍去小数末尾的0
	GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);
	xPos = m_FigRect.x;
	yPos = m_iClientHeight - m_FigRect.y;
	TextOutA(m_hDC, xPos - szText.cx-2, yPos - szText.cy, szCoord, strlen(szCoord));
	//m_ymax
	memset(szCoord, 0, 100);
	sprintf_s(szCoord, "%.3f", m_ymax);
	sscanf_s(szCoord, "%f", &fTemp);	//字符串转化为数字
	sprintf_s(szCoord, "%G", fTemp);	//将数字转化为字符串，并舍去小数末尾的0
	GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);
	xPos = m_FigRect.x;
	yPos = m_iClientHeight - m_FigRect.y- m_FigRect.height;
	TextOutA(m_hDC, xPos - szText.cx - 2, yPos - szText.cy/2, szCoord, strlen(szCoord));


}

void CPlot::SetRect()
{
	//窗口客户区
	GetClientRect(m_hWnd, &m_ClientRect);
	m_iClientWidth = m_ClientRect.right - m_ClientRect.left;
	m_iClientHeight = m_ClientRect.bottom - m_ClientRect.top;
	//绘图区信息
	m_FigRect.x = m_iClientWidth * 0.1;
	m_FigRect.y = m_iClientHeight * 0.1;
	m_FigRect.width = m_iClientWidth*0.8;
	m_FigRect.height = m_iClientHeight*0.8;
}

void CPlot::SetBGColor(GLColor ClrBG)
{
	m_ClrBG = ClrBG;
}

void CPlot::SetLineColor(GLColor ClrLine)
{
	m_ClrLine = ClrLine;
}

void CPlot::SetAxis(float xmin,float xmax,float ymin,float ymax)
{
	m_xmin = xmin;
	m_xmax = xmax;
	m_ymin = ymin;
	m_ymax = ymax;
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
		break;
	default:
		break;
	}
}

bool CPlot::GetYDataFromFile()	//获取绘图数据
{
	GetDataPath();
	bool bRet = false;

	do
	{
		if (strlen(m_szDataPath) == 0)
			break;
		fstream file1;
		file1.open(m_szDataPath, ios::in | ios::binary);
		if (!file1)
			break;
		//读取文件，写如文件，按行处理
		int num = 0;
		char ch[2];
		short sData = 0;
		while (!file1.eof())
		{
			memset(ch, 0, sizeof(ch));
			sData = 0;
			file1.read(ch, sizeof(ch));	//读取两个字节
			if (file1.good())			//防止到文件结束处多读取一次
			{
				memcpy(&sData, ch, 2);
				m_veDataY.push_back(sData);
				m_veDataX.push_back(num);
				num++;
				if (num == 199999)
					break;
			}	
		}
		file1.close();
		m_iDataLen = m_veDataY.size() / 2 * 2;	//获取数据长度,fft变换的数据长度必须为2的倍数，奇数个会出错！
		fft();
		bRet = true;
	} while (false);
	return bRet;

}

bool CPlot::GetXYDataFromFile()
{
	GetDataPath();
	bool bRet = false;
	
	do
	{
		if (strlen(m_szDataPath) == 0)
			break;
		fstream file1;
		file1.open(m_szDataPath, ios::in|ios::binary);
		if (!file1)
			break;
		//读取文件，写如文件，按行处理
		int num = 0;
		char ch[4];
		short IData = 0;
		short QData = 0;
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
				m_veDataX.push_back(double(IData) / (1 << 15));
				m_veDataY.push_back(double(QData) / (1 << 15));
				num++;
				if (num == 199999)
					break;
			}	
		}
		file1.close();
		m_iDataLen = m_veDataY.size()/2*2;	//获取数据长度,fft变换的数据长度必须为2的倍数，奇数个会出错！
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
	} while (false);
	return bRet;

}

void CPlot::GetMaxMinValue()	//获取数据的最大值或最小值
{
	if (m_veDataY.empty() || m_veDataX.empty() || (m_fft_pout == NULL))
		return;

	
	switch (m_enDrawType)
	{
	case en_Draw_Orignal:
	{
		//y轴
		std::vector<double>::iterator it_y = m_veDataY.begin();
		m_ymin = *(std::min_element(it_y, m_veDataY.end()));
		m_ymax = *(std::max_element(it_y, m_veDataY.end()));

		//x轴
		std::vector<double>::iterator it_x = m_veDataX.begin();
		m_xmin = *(std::min_element(it_x, m_veDataX.end()));
		m_xmax = *(std::max_element(it_x, m_veDataX.end()));
		break;
	}			
	case en_Draw_Spectrum:
	{
		//y轴
		m_ymin = *(std::min_element(m_fft_pabs, m_fft_pabs + ((m_iDataLen / 2 - 1) - 1)));
		m_ymax = *(std::max_element(m_fft_pabs, m_fft_pabs + ((m_iDataLen / 2 - 1) - 1)));
		
		//x轴
		std::vector<double>::iterator it_x = m_veDataX.begin();
		m_xmin = *(std::min_element(it_x, m_veDataX.end()));
		m_xmax = m_fs/2;
		break;
	}
		
	case en_Draw_Constellation:
	{
		//y轴
		std::vector<double>::iterator it_y = m_veDataY.begin();
		m_ymin = *(std::min_element(it_y, m_veDataY.end()));
		m_ymax = *(std::max_element(it_y, m_veDataY.end()));

		//x轴
		std::vector<double>::iterator it_x = m_veDataX.begin();
		m_xmin = *(std::min_element(it_x, m_veDataX.end()));
		m_xmax = *(std::max_element(it_x, m_veDataX.end()));
		break;
	}
	default:
		break;
	}

}

void CPlot::GetDataPath()
{
	memcpy(m_szDataPath, "bbb_transfer1.bin", strlen("bbb_transfer1.bin"));
	//memcpy(m_szDataPath, "ccc.bin", strlen("ccc.bin"));
}

void CPlot::OnReshape()
{
	//m_hDC = GetDC(m_hWnd);				//得到当前窗口的设备环境  
	GetClientRect(m_hWnd, &m_ClientRect);
	m_iClientWidth = m_ClientRect.right - m_ClientRect.left;
	m_iClientHeight = m_ClientRect.bottom - m_ClientRect.top;
	SetRect();
	wglDeleteContext(m_hRC);
	m_hRC = wglCreateContext(m_hDC);	//创建OpenGL绘图环境并创建一个指向OpenGL绘制环境的句柄  
	wglMakeCurrent(m_hDC, m_hRC);		//将传递过来的绘制环境设置为OpenGL将要进行绘制的当前绘制环境  

}

void CPlot::OnTranslate(const tagPOINT ptPre, const tagPOINT ptNow)	//平移图像,图像随着鼠标移动
{
	//平移x轴
	float fxOffSent = ScreentoOpenGLCoord(ptPre).x - ScreentoOpenGLCoord(ptNow).x;
	m_xmin += fxOffSent;
	m_xmax += fxOffSent;
	//平移y轴
	float fyOffSent = ScreentoOpenGLCoord(ptPre).y - ScreentoOpenGLCoord(ptNow).y;
	m_ymin += fyOffSent;
	m_ymax += fyOffSent;

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
		m_xmin = m_xmin / fTime;
		m_xmax = m_xmax / fTime;
		//缩放y轴
		/ *m_ymin = m_ymin / fTime;
		m_ymax = m_ymax / fTime;* /
	}
	else if (m_iScaleCount == 0)
	{
		if (fTime > 1)
		{
			m_iScaleCount++;
			//缩放x轴
			m_xmin = m_xmin / fTime;
			m_xmax = m_xmax / fTime;
			//缩放y轴
			/ *m_ymin = m_ymin / fTime;
			m_ymax = m_ymax / fTime;* /
		}	
	}*/
	/*
	//缩放x轴
	m_xmin = m_xmin / fTime;
	m_xmax = m_xmax / fTime;
	//缩放y轴
	m_ymin = m_ymin / fTime;
	m_ymax = m_ymax / fTime;
	*/

	//保持屏幕左下角不变的
	m_xmax = m_xmax - (m_xmin - m_xmin / fTime);
	m_ymax = m_ymax - (m_ymin - m_ymin / fTime);
}

bool CPlot::IsinRect(const tagPOINT pt)	//判断鼠标是否在绘图区域内
{
	if (pt.x<m_FigRect.x || 
		pt.x>m_FigRect.x+m_FigRect.width ||
		pt.y<m_iClientHeight-m_FigRect.y-m_FigRect.height ||
		pt.y>m_iClientHeight - m_FigRect.y)
		return false;
	return true;
}

void CPlot::ShowPosCoord(const tagPOINT pt)	//显示点的坐标值
{
	GLPoint Coord=ScreentoOpenGLCoord(pt);
	float fTemp1 = 0.0;
	float fTemp2 = 0.0;

	char szCoord[40];
	memset(szCoord, 0, sizeof(szCoord));
	sprintf_s(szCoord, "%.2f,%.2f", Coord.x, Coord.y);

	sscanf_s(szCoord, "%f,%f", &fTemp1,&fTemp2);	//字符串转化为数字
	sprintf_s(szCoord, "(%G,%G)", fTemp1,fTemp2);		//将数字转化为字符串，并舍去小数末尾的0


	TextOutA(m_hDC, pt.x, pt.y, szCoord, strlen(szCoord));

}

GLPoint CPlot::ScreentoOpenGLCoord(const tagPOINT Screen_pt)
{
	//调用该函数之前应该判断一下该点是否在绘图区
	GLPoint OpenGL_pt = { 0, 0 };
	OpenGL_pt.x = m_xmin + (Screen_pt.x - m_FigRect.x) / m_FigRect.width * (m_xmax - m_xmin);
	OpenGL_pt.y = m_ymin + ((m_iClientHeight - m_FigRect.y) - Screen_pt.y) / m_FigRect.height* (m_ymax - m_ymin);
	return OpenGL_pt;
}

tagPOINT CPlot::OpenGLtoScreenCoord(const GLPoint OpenGL_pt)
{
	//调用该函数之前应该判断一下该点是否在绘图区
	tagPOINT Screen_pt = { 0, 0 };
	Screen_pt.x = m_FigRect.x + (OpenGL_pt.x - m_xmin)/(m_xmax - m_xmin) * m_FigRect.width;
	Screen_pt.y = (m_iClientHeight - m_FigRect.y) - (OpenGL_pt.y - m_ymin)/(m_ymax - m_ymin) * m_FigRect.height;
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

	fftw_plan p;

	m_fft_pin = (double *)fftw_malloc(sizeof(double) * m_iDataLen);
	m_fft_pabs = (double *)fftw_malloc(sizeof(double) * (m_iDataLen/2-1));
	m_fft_pout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_iDataLen);

	if (!m_fft_pin || !m_fft_pout)
		return;
	
	//赋值
	for (int i = 0; i < m_iDataLen; i++)
	{
		m_fft_pin[i] = (m_veDataY[i] / (1 << 15)) / (m_iDataLen);	//考虑到实际的16bit ADC采集
	}

	// 傅里叶变换
	p = fftw_plan_dft_r2c_1d(m_iDataLen, m_fft_pin, m_fft_pout, FFTW_ESTIMATE);	//实数到复数，由Hermite对称性只取fs的一半

	fftw_execute(p);

	//赋值
	for (int i = 0; i <(m_iDataLen / 2 - 1); i++)
	{
		m_fft_pabs[i] = (sqrt(pow(m_fft_pout[i][0] , 2) + pow(m_fft_pout[i][1] , 2))) ;
		m_fft_pabs[i] = 20.0 * log10(m_fft_pabs[i]);	//log10(0)会出现莫名其妙的错误
	}


	// 释放资源
	fftw_destroy_plan(p);

}

void CPlot::DrawOrignal()
{
	glDisable(GL_LINE_STIPPLE);
	glLineWidth(2.0);
	glColor3f(m_ClrLine.R, m_ClrLine.G, m_ClrLine.B);
	std::vector<double>::iterator itX = m_veDataX.begin();
	std::vector<double>::iterator itY = m_veDataY.begin();
	glBegin(GL_LINE_STRIP);
	while (itX != m_veDataX.end())
	{
		glVertex2f(*itX, *itY);
		++itX;
		++itY;
	}
	glEnd();
}

void CPlot::DrawSpectrum()
{
	
	if (m_fft_pout == NULL)
		return;

	glDisable(GL_LINE_STIPPLE);
	glLineWidth(2.0);
	glColor3f(m_ClrLine.R, m_ClrLine.G, m_ClrLine.B);



	float fs = 100;	//100MHZ

	std::vector<double>::iterator itX = m_veDataX.begin();
	int i = 0;
	glBegin(GL_LINE_STRIP);
	while (i<(m_iDataLen / 2 - 1))
	{
		glVertex2f(i*fs/m_iDataLen, m_fft_pabs[i]);
		++i;
	}
	glEnd();
}

void CPlot::DrawConstellation()
{
	glDisable(GL_LINE_STIPPLE);
	glLineWidth(2.0);
	glColor3f(m_ClrLine.R, m_ClrLine.G, m_ClrLine.B);
	std::vector<double>::iterator itX = m_veDataX.begin();
	std::vector<double>::iterator itY = m_veDataY.begin();
	//glBegin(GL_POINTS);
	glBegin(GL_LINE_STRIP);
	while (itX != m_veDataX.end())
	{
		glVertex2f(*itX, *itY);
		++itX;
		++itY;
	}
	glEnd();
}