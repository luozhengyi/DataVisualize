#include "stdafx.h"
#include "Plot.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <atlstr.h>
#include<algorithm>
#include <math.h>
#include <valarray>
/***************************OpenGLͷ�ļ�/��̬��****************************/
#include <GL/gl.h>  
#include <GL/glu.h>  
#include <GL/glaux.h>  
#pragma comment(lib,"gl//glu32.lib")
#pragma comment(lib,"gl//glaux.lib")
#pragma comment(lib,"gl//opengl32.lib")
/************************************************************************/
CPlot::CPlot()
{
	//m_ClrBG = { 0.0f, 0.0f, 0.0f, 0.0f };		//��ͼ��������ɫ
	m_ClrBG = { 0.45f, 0.45f, 0.45f, 0.0f };
	//m_ClrWnd = { 0.51f, 0.51f, 0.51f, 0.0f };	//���屳����ɫ
	m_ClrWnd = { 1.0f, 1.0f, 1.0f, 0.0f };		//���屳����ɫ
	m_ClrLine = { 1.0f, 0.0f, 0.0f, 0.0f };		//��ͼ��ɫ
	m_ClrText = { 1.0f, 1.0f, 1.0f, 0.0f };		//���ְ�ɫ
	m_ClrAxis = { 0.0f, 0.0f, 0.0f, 0.0f };		//������ɫ

	m_enFigureType = E_FigureType_Normal;	//������ͨ��ͼ
	m_enDrawType = en_Draw_Spectrum;	//����ԭʼ�ź�ͼ
	m_enDataSource = en_From_File;			//Ĭ�ϴ��ļ���ȡ����

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
	//�����������
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
		SendMessage(m_hWnd, WM_PAINT, NULL, NULL);	//��WM_PAINT��Ϣ����Ͳ���˸����Ϊɶ��
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
		m_hDC = GetDC(m_hWnd);				//�õ���ǰ���ڵ��豸����  
		//���þ�������
		SetRect();
		//����win32�е�OpenGL����
		SetupPixelFormat();					//�������ظ�ʽ���ú���  
		m_hRC = wglCreateContext(m_hDC);	//����OpenGL��ͼ����������һ��ָ��OpenGL���ƻ����ľ��  
		wglMakeCurrent(m_hDC, m_hRC);		//�����ݹ����Ļ��ƻ�������ΪOpenGL��Ҫ���л��Ƶĵ�ǰ���ƻ���  
		//��ʼ��OpenGL��ͼ����
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
	glShadeModel(GL_SMOOTH);                 // ������Ӱƽ��  
	// ���ô��屳����ɫ
	glClearColor(m_ClrWnd.R, m_ClrWnd.G, m_ClrWnd.B, m_ClrWnd.A);
	glClearDepth(1.0f);                      // ������Ȼ���   
	glDepthFunc(GL_LEQUAL);                  // ������Ȳ��Ե�����  
	glEnable(GL_DEPTH_TEST);                 // ������Ȳ��� 
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);// ����ϵͳ��͸�ӽ�������

}

void CPlot::InitText()
{
	//��������
	m_hNewFont = CreateFontW(
		18,	//����ĸ߶�
		0,	//����Ŀ��
		0,	//�Ƕ�
		0,	//�Ƕ�
		0,	//����
		0,	//�Ƿ�б��
		0,	//���»���
		0,	//ɾ����
		GB2312_CHARSET,	//�ַ���,��������Ƚ���Ҫ
		0,	//����
		0,	//����
		0,	//����
		0,
		L"΢���ź�"	//���壬����Ҫ
		);
	m_hOldFont = SelectObject(m_hDC, m_hNewFont);
}

void CPlot::SetupPixelFormat() //Ϊ�豸�����������ظ�ʽ  
{
	int nPixelFormat; //���ظ�ʽ����  
	static PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), //���ݽṹ��С  
		1,							//�汾�ţ�����Ϊ1  
		PFD_DRAW_TO_WINDOW |			//֧�ִ���  
		PFD_SUPPORT_OPENGL |			//֧��OpenGL  
		PFD_DOUBLEBUFFER,			//֧��˫����  
		PFD_TYPE_RGBA,				//RGBA��ɫģʽ  
		32,							//32λ��ɫģʽ  
		0, 0, 0, 0, 0, 0,			//������ɫΪ����ʹ��  
		0,				//��alpha����  
		0,				//����ƫ��λ  
		0,				//���ۻ�����  
		0, 0, 0, 0,		//�����ۻ�λ  
		16,				//16λz-buffer��z���棩��С  
		0,				//��ģ�建��  
		0,				//�޸�������  
		PFD_MAIN_PLANE, //������ƽ��  
		0,				//������������  
		0, 0, 0 };		//���Բ�����ģ  
	//ѡ����ƥ������ظ�ʽ����������ֵ  
	nPixelFormat = ChoosePixelFormat(m_hDC, &pfd);
	//���û����豸�����ظ�ʽ  
	bool bRet = SetPixelFormat(m_hDC, nPixelFormat, &pfd);
}

void CPlot::Draw()
{
	// ���ô��屳����ɫ
	glClearColor(m_ClrWnd.R, m_ClrWnd.G, m_ClrWnd.B, m_ClrWnd.A);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //�����������Ȼ�����  

	//�����ӽǱ任
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//���ÿɼ���������
	gluOrtho2D(m_xmin, m_xmax, m_ymin , m_ymax);
	//����ͶӰ����Ļ�ϵ�����
	glViewport(m_FigRect.x, m_FigRect.y, m_FigRect.width, m_FigRect.height);

	//����ͼ�����ı���
	DrawBG();


	//������
	DrawAxis();

	//��������
	DrawData();

	glFlush();
	SwapBuffers(m_hDC);

	//����GDI��������̶ȣ���������󻭣���Ȼ�ᱻOpenGL����
	DrawText();
	//����
	if (m_bShowCoord)
		ShowPosCoord(m_tagPt);

	
}

void CPlot::DrawAxis()
{
	glColor3f(m_ClrAxis.R, m_ClrAxis.G, m_ClrAxis.B);

	//��������
	glLineStipple(1, 0x0F0F);	//��������ģʽ
	glEnable(GL_LINE_STIPPLE);	//��������ģʽ
	glLineWidth(1.0f);			//�����߿��
	glBegin(GL_LINES);

	m_xscale = (m_xmax - m_xmin) / 10.0;
	m_yscale = (m_ymax - m_ymin) / 10.0;

	//x�᷽������
	float y = m_ymin;
	for (; y <= m_ymax; y+=m_yscale)
	{
		glVertex2f(m_xmin, y);
		glVertex2f(m_xmax, y);
	}
	//y�᷽������
	float x = m_xmin;
	for (; x<= m_xmax; x+=m_xscale)
	{
		glVertex2f(x, m_ymin);
		glVertex2f(x, m_ymax);
	}
	glEnd();
	glDisable(GL_LINE_STIPPLE);	//�ر�����ģʽ

	
	//���Ʊ߿�
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	//x�᷽��߿�
	glVertex2f(m_xmin, m_ymin);
	glVertex2f(m_xmax, m_ymin);
	glVertex2f(m_xmin, m_ymax);
	glVertex2f(m_xmax, m_ymax);
	//y�᷽��߿�
	glVertex2f(m_xmin, m_ymin);
	glVertex2f(m_xmin, m_ymax);
	glVertex2f(m_xmax, m_ymin);
	glVertex2f(m_xmax, m_ymax);
	glEnd();
}

void CPlot::DrawBG()
{
	//����ͼ�����ı���
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //�����������Ȼ�����  
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
	char szCoord[100];	//����̶�
	memset(szCoord, 0, 100);

	//�����ַ���ռ�Ŀ�����ء�
	SIZE szText;
	

	//RECT rect = { 10, 10, 50, 26 };	//Ĭ�����彫��һ����ĸ����Ϊ8�����أ���Ϊ16������
	//::DrawTextA(m_hDC, "hello", 5,&rect, 0);

	//��������̶�
	float fTemp = 0.0;
	int xPos = 0;
	int yPos = 0;
	//m_xmin
	memset(szCoord, 0, 100);
	sprintf_s(szCoord, "%.3f", m_xmin);	//������(����С�������λ)ת��Ϊ�ַ�����
	sscanf_s(szCoord, "%f", &fTemp);	//�ַ���ת��Ϊ����
	sprintf_s(szCoord, "%G", fTemp);	//������ת��Ϊ�ַ���������ȥС��ĩβ��0
	GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);	//�����ַ�����ռ�õ�����
	xPos = m_FigRect.x;
	yPos = m_iClientHeight - m_FigRect.y;
	TextOutA(m_hDC, xPos, yPos +3 , szCoord, strlen(szCoord));
	//m_xmax
	memset(szCoord, 0, 100);
	sprintf_s(szCoord, "%.3f", m_xmax);
	sscanf_s(szCoord, "%f", &fTemp);	//�ַ���ת��Ϊ����
	sprintf_s(szCoord, "%G", fTemp);	//������ת��Ϊ�ַ���������ȥС��ĩβ��0
	xPos = m_FigRect.x + m_FigRect.width;
	yPos = m_iClientHeight - m_FigRect.y;
	TextOutA(m_hDC, xPos - szText.cx/2, yPos + 3, szCoord, strlen(szCoord));
	//m_ymin
	memset(szCoord, 0, 100);
	sprintf_s(szCoord, "%.3f", m_ymin);
	sscanf_s(szCoord, "%f", &fTemp);	//�ַ���ת��Ϊ����
	sprintf_s(szCoord, "%G", fTemp);	//������ת��Ϊ�ַ���������ȥС��ĩβ��0
	GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);
	xPos = m_FigRect.x;
	yPos = m_iClientHeight - m_FigRect.y;
	TextOutA(m_hDC, xPos - szText.cx-2, yPos - szText.cy, szCoord, strlen(szCoord));
	//m_ymax
	memset(szCoord, 0, 100);
	sprintf_s(szCoord, "%.3f", m_ymax);
	sscanf_s(szCoord, "%f", &fTemp);	//�ַ���ת��Ϊ����
	sprintf_s(szCoord, "%G", fTemp);	//������ת��Ϊ�ַ���������ȥС��ĩβ��0
	GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);
	xPos = m_FigRect.x;
	yPos = m_iClientHeight - m_FigRect.y- m_FigRect.height;
	TextOutA(m_hDC, xPos - szText.cx - 2, yPos - szText.cy/2, szCoord, strlen(szCoord));


}

void CPlot::SetRect()
{
	//���ڿͻ���
	GetClientRect(m_hWnd, &m_ClientRect);
	m_iClientWidth = m_ClientRect.right - m_ClientRect.left;
	m_iClientHeight = m_ClientRect.bottom - m_ClientRect.top;
	//��ͼ����Ϣ
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

bool CPlot::GetYDataFromFile()	//��ȡ��ͼ����
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
		//��ȡ�ļ���д���ļ������д���
		int num = 0;
		char ch[2];
		short sData = 0;
		while (!file1.eof())
		{
			memset(ch, 0, sizeof(ch));
			sData = 0;
			file1.read(ch, sizeof(ch));	//��ȡ�����ֽ�
			if (file1.good())			//��ֹ���ļ����������ȡһ��
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
		m_iDataLen = m_veDataY.size() / 2 * 2;	//��ȡ���ݳ���,fft�任�����ݳ��ȱ���Ϊ2�ı����������������
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
		//��ȡ�ļ���д���ļ������д���
		int num = 0;
		char ch[4];
		short IData = 0;
		short QData = 0;
		while (!file1.eof())
		{
			memset(ch, 0, sizeof(ch));
			IData = 0;
			QData = 0;
			file1.read(ch, sizeof(ch));	//��ȡ4���ֽ�
			if (file1.good())			//��ֹ���ļ����������ȡһ��
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
		m_iDataLen = m_veDataY.size()/2*2;	//��ȡ���ݳ���,fft�任�����ݳ��ȱ���Ϊ2�ı����������������
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

void CPlot::GetMaxMinValue()	//��ȡ���ݵ����ֵ����Сֵ
{
	if (m_veDataY.empty() || m_veDataX.empty() || (m_fft_pout == NULL))
		return;

	
	switch (m_enDrawType)
	{
	case en_Draw_Orignal:
	{
		//y��
		std::vector<double>::iterator it_y = m_veDataY.begin();
		m_ymin = *(std::min_element(it_y, m_veDataY.end()));
		m_ymax = *(std::max_element(it_y, m_veDataY.end()));

		//x��
		std::vector<double>::iterator it_x = m_veDataX.begin();
		m_xmin = *(std::min_element(it_x, m_veDataX.end()));
		m_xmax = *(std::max_element(it_x, m_veDataX.end()));
		break;
	}			
	case en_Draw_Spectrum:
	{
		//y��
		m_ymin = *(std::min_element(m_fft_pabs, m_fft_pabs + ((m_iDataLen / 2 - 1) - 1)));
		m_ymax = *(std::max_element(m_fft_pabs, m_fft_pabs + ((m_iDataLen / 2 - 1) - 1)));
		
		//x��
		std::vector<double>::iterator it_x = m_veDataX.begin();
		m_xmin = *(std::min_element(it_x, m_veDataX.end()));
		m_xmax = m_fs/2;
		break;
	}
		
	case en_Draw_Constellation:
	{
		//y��
		std::vector<double>::iterator it_y = m_veDataY.begin();
		m_ymin = *(std::min_element(it_y, m_veDataY.end()));
		m_ymax = *(std::max_element(it_y, m_veDataY.end()));

		//x��
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
	//m_hDC = GetDC(m_hWnd);				//�õ���ǰ���ڵ��豸����  
	GetClientRect(m_hWnd, &m_ClientRect);
	m_iClientWidth = m_ClientRect.right - m_ClientRect.left;
	m_iClientHeight = m_ClientRect.bottom - m_ClientRect.top;
	SetRect();
	wglDeleteContext(m_hRC);
	m_hRC = wglCreateContext(m_hDC);	//����OpenGL��ͼ����������һ��ָ��OpenGL���ƻ����ľ��  
	wglMakeCurrent(m_hDC, m_hRC);		//�����ݹ����Ļ��ƻ�������ΪOpenGL��Ҫ���л��Ƶĵ�ǰ���ƻ���  

}

void CPlot::OnTranslate(const tagPOINT ptPre, const tagPOINT ptNow)	//ƽ��ͼ��,ͼ����������ƶ�
{
	//ƽ��x��
	float fxOffSent = ScreentoOpenGLCoord(ptPre).x - ScreentoOpenGLCoord(ptNow).x;
	m_xmin += fxOffSent;
	m_xmax += fxOffSent;
	//ƽ��y��
	float fyOffSent = ScreentoOpenGLCoord(ptPre).y - ScreentoOpenGLCoord(ptNow).y;
	m_ymin += fyOffSent;
	m_ymax += fyOffSent;

}

void CPlot::OnScale(float fTime)		
{//����ͼ��,������һֱ��С��������һֱ�Ŵ�
	/*if (m_iScaleCount >= 1)
	{
		if (fTime > 1)
			m_iScaleCount++;
		else
			m_iScaleCount--;
		//����x��
		m_xmin = m_xmin / fTime;
		m_xmax = m_xmax / fTime;
		//����y��
		/ *m_ymin = m_ymin / fTime;
		m_ymax = m_ymax / fTime;* /
	}
	else if (m_iScaleCount == 0)
	{
		if (fTime > 1)
		{
			m_iScaleCount++;
			//����x��
			m_xmin = m_xmin / fTime;
			m_xmax = m_xmax / fTime;
			//����y��
			/ *m_ymin = m_ymin / fTime;
			m_ymax = m_ymax / fTime;* /
		}	
	}*/
	/*
	//����x��
	m_xmin = m_xmin / fTime;
	m_xmax = m_xmax / fTime;
	//����y��
	m_ymin = m_ymin / fTime;
	m_ymax = m_ymax / fTime;
	*/

	//������Ļ���½ǲ����
	m_xmax = m_xmax - (m_xmin - m_xmin / fTime);
	m_ymax = m_ymax - (m_ymin - m_ymin / fTime);
}

bool CPlot::IsinRect(const tagPOINT pt)	//�ж�����Ƿ��ڻ�ͼ������
{
	if (pt.x<m_FigRect.x || 
		pt.x>m_FigRect.x+m_FigRect.width ||
		pt.y<m_iClientHeight-m_FigRect.y-m_FigRect.height ||
		pt.y>m_iClientHeight - m_FigRect.y)
		return false;
	return true;
}

void CPlot::ShowPosCoord(const tagPOINT pt)	//��ʾ�������ֵ
{
	GLPoint Coord=ScreentoOpenGLCoord(pt);
	float fTemp1 = 0.0;
	float fTemp2 = 0.0;

	char szCoord[40];
	memset(szCoord, 0, sizeof(szCoord));
	sprintf_s(szCoord, "%.2f,%.2f", Coord.x, Coord.y);

	sscanf_s(szCoord, "%f,%f", &fTemp1,&fTemp2);	//�ַ���ת��Ϊ����
	sprintf_s(szCoord, "(%G,%G)", fTemp1,fTemp2);		//������ת��Ϊ�ַ���������ȥС��ĩβ��0


	TextOutA(m_hDC, pt.x, pt.y, szCoord, strlen(szCoord));

}

GLPoint CPlot::ScreentoOpenGLCoord(const tagPOINT Screen_pt)
{
	//���øú���֮ǰӦ���ж�һ�¸õ��Ƿ��ڻ�ͼ��
	GLPoint OpenGL_pt = { 0, 0 };
	OpenGL_pt.x = m_xmin + (Screen_pt.x - m_FigRect.x) / m_FigRect.width * (m_xmax - m_xmin);
	OpenGL_pt.y = m_ymin + ((m_iClientHeight - m_FigRect.y) - Screen_pt.y) / m_FigRect.height* (m_ymax - m_ymin);
	return OpenGL_pt;
}

tagPOINT CPlot::OpenGLtoScreenCoord(const GLPoint OpenGL_pt)
{
	//���øú���֮ǰӦ���ж�һ�¸õ��Ƿ��ڻ�ͼ��
	tagPOINT Screen_pt = { 0, 0 };
	Screen_pt.x = m_FigRect.x + (OpenGL_pt.x - m_xmin)/(m_xmax - m_xmin) * m_FigRect.width;
	Screen_pt.y = (m_iClientHeight - m_FigRect.y) - (OpenGL_pt.y - m_ymin)/(m_ymax - m_ymin) * m_FigRect.height;
	return Screen_pt;
}

void CPlot::DestroyPlot()
{
	wglMakeCurrent(m_hDC, NULL);	//��ԭ��ǰDC�е�RCΪNULL
	if (m_hRC != INVALID_HANDLE_VALUE)
	{
		wglDeleteContext(m_hRC);		//ɾ��RC
	}
	if (m_hNewFont)
		DeleteObject(m_hNewFont);
	if (m_hOldFont)
		SelectObject(m_hDC, m_hOldFont);
	ReleaseDC(m_hWnd, m_hDC);

}


/******************************�źŴ�����*****************************/
void CPlot::fft()
{
	//fft֮ǰ���ͷ��ڴ棬��ֹ�ڴ�й©
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
	
	//��ֵ
	for (int i = 0; i < m_iDataLen; i++)
	{
		m_fft_pin[i] = (m_veDataY[i] / (1 << 15)) / (m_iDataLen);	//���ǵ�ʵ�ʵ�16bit ADC�ɼ�
	}

	// ����Ҷ�任
	p = fftw_plan_dft_r2c_1d(m_iDataLen, m_fft_pin, m_fft_pout, FFTW_ESTIMATE);	//ʵ������������Hermite�Գ���ֻȡfs��һ��

	fftw_execute(p);

	//��ֵ
	for (int i = 0; i <(m_iDataLen / 2 - 1); i++)
	{
		m_fft_pabs[i] = (sqrt(pow(m_fft_pout[i][0] , 2) + pow(m_fft_pout[i][1] , 2))) ;
		m_fft_pabs[i] = 20.0 * log10(m_fft_pabs[i]);	//log10(0)�����Ī������Ĵ���
	}


	// �ͷ���Դ
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