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
/***************************OpenGLͷ�ļ�/��̬��****************************/
#include <GL/gl.h>  
#include <GL/glu.h>  
#include <GL/glaux.h>  
#pragma comment(lib,"gl//glu32.lib")
#pragma comment(lib,"gl//glaux.lib")
#pragma comment(lib,"gl//opengl32.lib")
/************************************************************************/

#pragma comment(lib,"ws2_32.lib")


//��̬��������
int CPlot::objNum = 0;	//��ʵ��������

enDataSource CPlot::m_enDataSource = en_From_File;								//������Դ
wchar_t*	 CPlot::m_szDataPath = new wchar_t[NAME_LEN]();						//�����ļ�·��
int			 CPlot::m_iDataLen = 0;												//���ݳ���
INT16*		 CPlot::m_pDataX = new INT16[g_sockFrameLen / 2]();					//��������
INT16*		 CPlot::m_pDataY = new INT16[g_sockFrameLen / 2]();					//��������
enDrawStatus CPlot::m_enDrawStatus = E_Draw_Fresh;								//E_Draw_Fresh����ʾ����ʵʱ���ݣ�
stFile		 CPlot::m_stFile;													//���ݲɼ���Ϣ�ṹ��


float			CPlot::m_fs = 20;				//����Ƶ�ʣ���λMHZ
double*			CPlot::m_fft_pin = NULL;		//fft�����ź�
double*			CPlot::m_fft_pabs = NULL;		//fft����źŵľ���ֵ
fftw_complex*	CPlot::m_fft_pout = NULL;		//fft������ź�



/************************************************************************/
/* TCP���ջص�����                                                  */
/************************************************************************/
//const int g_sockFrameLen = sizeof(short) * 21413580;
const int g_sockFrameLen = sizeof(short) * 262144;
//const int g_sockFrameLen = sizeof(short) * 524288;
char ato_buff[g_sockFrameLen];	//һ֡����
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
			//��������
			if (recvcount == 0)
				g_mtx.lock();
			irecv = recv(sockClient, (char*)ato_buff + recvcount, g_sockFrameLen - recvcount, 0);
			if (irecv == 0 || irecv == SOCKET_ERROR)
			{
				*(SOCKET*)p = INVALID_SOCKET;
				if (recvcount >= 0)	//����δ����һ֡��tcp�����ж�
					g_mtx.unlock();
				_endthread();//ɾ���߳�
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
	m_enFigureType = E_FigureType_Normal;	//������ͨ��ͼ
	m_enDrawType = en_Draw_Orignal;			//����ԭʼ�ź�ͼ

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
		//�����������
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

	//��ԭ��ͼ����
	DestroyPlot();

	//DSP��Դ�ͷ�
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
		
		//���������Ӵ���
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			if (it->second == PlotWndProc && it->first->GetWndStatus() == E_WND_Show)	//֪ͨ��ͼ���ڽ��л�ͼ
				SendMessage(it->first->GetWnd(), WM_PAINT, NULL, NULL);	//��WM_PAINT��Ϣ����Ͳ���˸����Ϊɶ��
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
		m_hDC = GetDC(m_hWnd);				//�õ���ǰ���ڵ��豸����  
		//����OpenGL��ͼ�ľ�������
		SetFigRect();
		//����win32�е�OpenGL����
		SetupPixelFormat();					//�������ظ�ʽ���ú���  
		m_hRC = wglCreateContext(m_hDC);	//����OpenGL��ͼ����������һ��ָ��OpenGL���ƻ����ľ��  
		wglMakeCurrent(m_hDC, m_hRC);		//�����ݹ����Ļ��ƻ�������ΪOpenGL��Ҫ���л��Ƶĵ�ǰ���ƻ���  
		//��ʼ��OpenGL��ͼ����
		InitGL();
		InitText();



		if (m_enDataSource != en_From_Sock)
		{
			GetData();
			GetMaxMinValue();
		}

		//ֻ��Ҫ��һ����ͼ���ڴ�����ʱ�����С�
		childwndmap::iterator it = g_WndManager.GetChildWndMap()->begin();
		while (it != g_WndManager.GetChildWndMap()->end())
		{
			CPlot *pPlot = it->first->GetPlotObj();
			CPlot *pPlot1 = this;
			if ((it->first->GetWnd()) == hWnd)
			{
				if (it->first->GetWndID() == E_WND_Plot_Right)
				{
					ClearTimer();	//���������ж�ʱ��
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
	glShadeModel(GL_SMOOTH);                 // ������Ӱƽ��  
	// ���ô��屳����ɫ
	glClearColor(m_stFigClrInfo.ClrWnd.R, m_stFigClrInfo.ClrWnd.G, m_stFigClrInfo.ClrWnd.B, m_stFigClrInfo.ClrWnd.A);
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
	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), //���ݽṹ��С  
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
	wglMakeCurrent(m_hDC, m_hRC);	// MDI �ڶ���Ӵ���֮�����ص�������ʱҪ�ӵ�һ�仰����Ȼֻ����һ���Ӵ������������ƣ������Ķ������


	// ���ô��屳����ɫ
	glClearColor(m_stFigClrInfo.ClrWnd.R, m_stFigClrInfo.ClrWnd.G, m_stFigClrInfo.ClrWnd.B, m_stFigClrInfo.ClrWnd.A);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //�����������Ȼ�����  

	//�����ӽǱ任
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//���ÿɼ���������
	gluOrtho2D(m_stAxisInfo.xmin, m_stAxisInfo.xmax, m_stAxisInfo.ymin, m_stAxisInfo.ymax);
	//����ͶӰ����Ļ�ϵ�����
	glViewport(m_FigRect.x, m_FigRect.y, m_FigRect.width, m_FigRect.heigh);


	//����ͼ�����ı���
	DrawBG();



	//������
	DrawAxis();

	//g_mtx.lock();
	//��������
	DrawData();
	//g_mtx.unlock();

	glFlush();
	bool bRet = SwapBuffers(m_hDC);

	//����GDI��������̶ȣ���������󻭣���Ȼ�ᱻOpenGL����
	DrawText();
	//����
	if (m_bShowCoord)
		ShowPosCoord(m_tagPt);



}

void CPlot::DrawAxis()
{
	glColor3f(m_stFigClrInfo.ClrAxis.R, m_stFigClrInfo.ClrAxis.G, m_stFigClrInfo.ClrAxis.B);

	//��������
	glLineStipple(1, 0x0F0F);	//��������ģʽ
	glEnable(GL_LINE_STIPPLE);	//��������ģʽ
	glLineWidth(1.0f);			//�����߿��
	glBegin(GL_LINES);

	m_stAxisInfo.xscale = (m_stAxisInfo.xmax - m_stAxisInfo.xmin) / 10.0;
	m_stAxisInfo.yscale = (m_stAxisInfo.ymax - m_stAxisInfo.ymin) / 10.0;

	//y�᷽������
	float y = m_stAxisInfo.ymin;
	for (; y <= m_stAxisInfo.ymax; y += m_stAxisInfo.yscale)
	{
		glVertex2f(m_stAxisInfo.xmin, y);
		glVertex2f(m_stAxisInfo.xmax, y);
	}
	//x�᷽������
	float x = m_stAxisInfo.xmin;
	for (; x <= m_stAxisInfo.xmax; x += m_stAxisInfo.xscale)
	{
		glVertex2f(x, m_stAxisInfo.ymin);
		glVertex2f(x, m_stAxisInfo.ymax);
	}
	glEnd();
	glDisable(GL_LINE_STIPPLE);	//�ر�����ģʽ


	//���Ʊ߿�
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	//x�᷽��߿�
	glVertex2f(m_stAxisInfo.xmin, m_stAxisInfo.ymin);
	glVertex2f(m_stAxisInfo.xmax, m_stAxisInfo.ymin);
	glVertex2f(m_stAxisInfo.xmin, m_stAxisInfo.ymax);
	glVertex2f(m_stAxisInfo.xmax, m_stAxisInfo.ymax);
	//y�᷽��߿�
	glVertex2f(m_stAxisInfo.xmin, m_stAxisInfo.ymin);
	glVertex2f(m_stAxisInfo.xmin, m_stAxisInfo.ymax);
	glVertex2f(m_stAxisInfo.xmax, m_stAxisInfo.ymin);
	glVertex2f(m_stAxisInfo.xmax, m_stAxisInfo.ymax);
	glEnd();
}

void CPlot::DrawBG()
{
	//����ͼ�����ı���
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //�����������Ȼ�����  
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

	fTemp = m_stAxisInfo.xmin;
	for (int i = 0; fTemp <= m_stAxisInfo.xmax + m_stAxisInfo.xscale/100.0; fTemp += m_stAxisInfo.xscale, ++i)
	{
		//stAxisInfo.xmin
		memset(szCoord, 0, 100);
		sprintf_s(szCoord, "%.3f", fTemp);	//������(����С�������λ)ת��Ϊ�ַ�����
		fTemp = 0.0;
		sscanf_s(szCoord, "%f", &fTemp);	//�ַ���ת��Ϊ����
		sprintf_s(szCoord, "%G", fTemp);	//������ת��Ϊ�ַ���������ȥС��ĩβ��0
		GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);	//�����ַ�����ռ�õ�����
		xPos = m_FigRect.x +i*m_FigRect.width/10.0;
		yPos = m_iClientHeight - m_FigRect.y;
		TextOutA(m_hDC, xPos - szText.cx / 2, yPos + 3, szCoord, strlen(szCoord));
	}
	
	fTemp = m_stAxisInfo.ymin;
	for (int i = 0; fTemp <= m_stAxisInfo.ymax + m_stAxisInfo.yscale / 100.0; fTemp += m_stAxisInfo.yscale, ++i)
	{
		memset(szCoord, 0, 100);
		sprintf_s(szCoord, "%.3f", fTemp);	//����ת��Ϊ�ַ���
		fTemp = 0.0;
		sscanf_s(szCoord, "%f", &fTemp);	//�ַ���ת��Ϊ����
		sprintf_s(szCoord, "%G", fTemp);	//������ת��Ϊ�ַ���������ȥС��ĩβ��0
		GetTextExtentPoint32A(m_hDC, szCoord, strlen(szCoord), &szText);
		xPos = m_FigRect.x;
		yPos = m_iClientHeight - m_FigRect.y - i*m_FigRect.heigh / 10.0;
		TextOutA(m_hDC, xPos - szText.cx - 4, yPos - szText.cy / 2, szCoord, strlen(szCoord));
	}

}

void CPlot::SetFigRect()
{
	//���ڿͻ���
	GetClientRect(m_hWnd, &m_ClientRect);
	m_iClientWidth = m_ClientRect.right - m_ClientRect.left;
	m_iClientHeight = m_ClientRect.bottom - m_ClientRect.top;
	//��ͼ����Ϣ
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

bool CPlot::GetYDataFromFile()	//��ȡ��ͼ����
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
		//��ȡ�ļ���д���ļ������д���
		int num = 0;
		char ch[2];
		short sData = 0;
		memset(m_pDataX, 0, g_sockFrameLen);
		memset(m_pDataY, 0, g_sockFrameLen);
		while (!file1.eof())
		{
			memset(ch, 0, sizeof(ch));
			sData = 0;
			file1.read(ch, sizeof(ch));	//��ȡ�����ֽ�
			if (file1.good())			//��ֹ���ļ����������ȡһ��
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
		m_iDataLen = num / 2 * 2;	//��ȡ���ݳ���,fft�任�����ݳ��ȱ���Ϊ2�ı����������������
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
		//��ȡ�ļ���д���ļ������д���
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
			file1.read(ch, sizeof(ch));	//��ȡ4���ֽ�
			if (file1.good())			//��ֹ���ļ����������ȡһ��
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
		m_iDataLen = num / 2 * 2;	//��ȡ���ݳ���,fft�任�����ݳ��ȱ���Ϊ2�ı����������������
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

void CPlot::GetMaxMinValue()	//��ȡ���ݵ����ֵ����Сֵ
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


		//y��
		short tempYMaxVal = -32768;
		short tempYMinVal = 32767;
		for (int i = 0; i < iLen; i++)
		{
			if (m_pDataY[i] > tempYMaxVal)
				tempYMaxVal = m_pDataY[i];
			if (m_pDataY[i] < tempYMinVal)
				tempYMinVal = m_pDataY[i];
		}

		if (tempYMinVal == 0 && tempYMaxVal == 0)		//�տ�ʼʱ��socket���Ӻ��ˣ����ǲ�û�д����ݹ���
		{
			m_stAxisInfo.ymin = 0;
			m_stAxisInfo.ymax = 1;
		}
		//��� stAxisInfo.ymin = stAxisInfo.ymax = 0  �ᵼ�³�����
		else
		{
			m_stAxisInfo.ymin = tempYMinVal;
			m_stAxisInfo.ymax = tempYMaxVal;
		}

		//x��
		m_stAxisInfo.xmin = 0;
		m_stAxisInfo.xmax = iLen;
		break;
	}
	case en_Draw_Spectrum:
	{
		//y��
		m_stAxisInfo.ymin = -140;
		m_stAxisInfo.ymax = 0;


		//x��
		m_stAxisInfo.xmin = 0;
		m_stAxisInfo.xmax = m_fs / 2;
		break;
	}

	case en_Draw_Constellation:
	{
		//y��
		//std::vector<double>::iterator it_y = m_veDataY.begin();
		//stAxisInfo.ymin = *(std::min_element(it_y, m_veDataY.end()));
		//stAxisInfo.ymax = *(std::max_element(it_y, m_veDataY.end()));
		m_stAxisInfo.ymin = -1.2;
		m_stAxisInfo.ymax = 1.2;

		////x��
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

		//������Socket�����ԶϿ������Զ�����
		closesocket(m_stSockInfo.sockSrv);
		WSACleanup();
			

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			::MessageBoxA(NULL, "WSA Startup Failed!", "Error", NULL);
			break;
		}

		//�������ڼ������׽���,������˵��׽���
		m_stSockInfo.sockSrv = socket(AF_INET, SOCK_STREAM, 0);

		SOCKADDR_IN addrSrv;

		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(m_stSockInfo.sockPort); //1024���ϵĶ˿ں�
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

		//�ȴ��ͻ�������,�Ὣ ��ʱ���߳� ����
		m_stSockInfo.sockClient = accept(m_stSockInfo.sockSrv, (SOCKADDR *)&addrClient, &len);
		if (m_stSockInfo.sockClient == INVALID_SOCKET)
		{
			//::MessageBoxA(NULL, "Accept Failed!", "Error", NULL);
			break;
		}

		//���������߳�
		_beginthread(CallBack_RecvProc, 0, &m_stSockInfo.sockClient); //������ʱ���߳����������ж�ʱ��

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
	//m_hDC = GetDC(m_hWnd);				//�õ���ǰ���ڵ��豸����  
	GetClientRect(m_hWnd, &m_ClientRect);
	m_iClientWidth = m_ClientRect.right - m_ClientRect.left;
	m_iClientHeight = m_ClientRect.bottom - m_ClientRect.top;
	SetFigRect();
	wglDeleteContext(m_hRC);
	m_hRC = wglCreateContext(m_hDC);	//����OpenGL��ͼ����������һ��ָ��OpenGL���ƻ����ľ��  
	wglMakeCurrent(m_hDC, m_hRC);		//�����ݹ����Ļ��ƻ�������ΪOpenGL��Ҫ���л��Ƶĵ�ǰ���ƻ���  

}

void CPlot::OnTranslate(const tagPOINT ptPre, const tagPOINT ptNow)	//ƽ��ͼ��,ͼ����������ƶ�
{
	//ƽ��x��
	float fxOffSent = ScreentoOpenGLCoord(ptPre).x - ScreentoOpenGLCoord(ptNow).x;
	m_stAxisInfo.xmin += fxOffSent;
	m_stAxisInfo.xmax += fxOffSent;
	//ƽ��y��
	float fyOffSent = ScreentoOpenGLCoord(ptPre).y - ScreentoOpenGLCoord(ptNow).y;
	m_stAxisInfo.ymin += fyOffSent;
	m_stAxisInfo.ymax += fyOffSent;

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
	stAxisInfo.xmin = stAxisInfo.xmin / fTime;
	stAxisInfo.xmax = stAxisInfo.xmax / fTime;
	//����y��
	/ *stAxisInfo.ymin = stAxisInfo.ymin / fTime;
	stAxisInfo.ymax = stAxisInfo.ymax / fTime;* /
	}
	else if (m_iScaleCount == 0)
	{
	if (fTime > 1)
	{
	m_iScaleCount++;
	//����x��
	stAxisInfo.xmin = stAxisInfo.xmin / fTime;
	stAxisInfo.xmax = stAxisInfo.xmax / fTime;
	//����y��
	/ *stAxisInfo.ymin = stAxisInfo.ymin / fTime;
	stAxisInfo.ymax = stAxisInfo.ymax / fTime;* /
	}
	}*/
	/*
	//����x��
	stAxisInfo.xmin = stAxisInfo.xmin / fTime;
	stAxisInfo.xmax = stAxisInfo.xmax / fTime;
	//����y��
	stAxisInfo.ymin = stAxisInfo.ymin / fTime;
	stAxisInfo.ymax = stAxisInfo.ymax / fTime;
	*/

	//������Ļ���½ǲ����
	m_stAxisInfo.xmax = m_stAxisInfo.xmax - (m_stAxisInfo.xmin - m_stAxisInfo.xmin / fTime);
	//stAxisInfo.ymax = stAxisInfo.ymax - (stAxisInfo.ymin - stAxisInfo.ymin / fTime);
}

bool CPlot::IsinRect(const tagPOINT pt)	//�ж�����Ƿ��ڻ�ͼ������
{
	if (pt.x<m_FigRect.x ||
		pt.x>m_FigRect.x + m_FigRect.width ||
		pt.y<m_iClientHeight - m_FigRect.y - m_FigRect.heigh ||
		pt.y>m_iClientHeight - m_FigRect.y)
		return false;
	return true;
}

void CPlot::ShowPosCoord(const tagPOINT pt)	//��ʾ�������ֵ
{
	GLPoint Coord = ScreentoOpenGLCoord(pt);
	float fTemp1 = 0.0;
	float fTemp2 = 0.0;

	char szCoord[40];
	memset(szCoord, 0, sizeof(szCoord));
	sprintf_s(szCoord, "%.2f,%.2f", Coord.x, Coord.y);

	sscanf_s(szCoord, "%f,%f", &fTemp1, &fTemp2);	//�ַ���ת��Ϊ����
	sprintf_s(szCoord, "(%G,%G)", fTemp1, fTemp2);		//������ת��Ϊ�ַ���������ȥС��ĩβ��0


	TextOutA(m_hDC, pt.x, pt.y, szCoord, strlen(szCoord));

}

GLPoint CPlot::ScreentoOpenGLCoord(const tagPOINT Screen_pt)
{
	//���øú���֮ǰӦ���ж�һ�¸õ��Ƿ��ڻ�ͼ��
	GLPoint OpenGL_pt = { 0, 0 };
	OpenGL_pt.x = m_stAxisInfo.xmin + (Screen_pt.x - m_FigRect.x) / m_FigRect.width * (m_stAxisInfo.xmax - m_stAxisInfo.xmin);
	OpenGL_pt.y = m_stAxisInfo.ymin + ((m_iClientHeight - m_FigRect.y) - Screen_pt.y) / m_FigRect.heigh* (m_stAxisInfo.ymax - m_stAxisInfo.ymin);
	return OpenGL_pt;
}

tagPOINT CPlot::OpenGLtoScreenCoord(const GLPoint OpenGL_pt)
{
	//���øú���֮ǰӦ���ж�һ�¸õ��Ƿ��ڻ�ͼ��
	tagPOINT Screen_pt = { 0, 0 };
	Screen_pt.x = m_FigRect.x + (OpenGL_pt.x - m_stAxisInfo.xmin) / (m_stAxisInfo.xmax - m_stAxisInfo.xmin) * m_FigRect.width;
	Screen_pt.y = (m_iClientHeight - m_FigRect.y) - (OpenGL_pt.y - m_stAxisInfo.ymin) / (m_stAxisInfo.ymax - m_stAxisInfo.ymin) * m_FigRect.heigh;
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

	//OnTimer�̱߳�RecvProc�߳������������Ե�һ��ȡ����Ϊ��
	if (m_iDataLen == 0)
		return;

	fftw_plan p;

	m_fft_pin = (double *)fftw_malloc(sizeof(double) * m_iDataLen);
	m_fft_pabs = (double *)fftw_malloc(sizeof(double) * (m_iDataLen / 2 - 1));
	m_fft_pout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_iDataLen);

	if (!m_fft_pin || !m_fft_pout)
		return;

	//��ֵ
	for (int i = 0; i < m_iDataLen; i++)
	{
		m_fft_pin[i] = ((double)m_pDataY[i] / (1 << 15)) / (m_iDataLen);	//���ǵ�ʵ�ʵ�16bit ADC�ɼ�
	}

	// ����Ҷ�任
	p = fftw_plan_dft_r2c_1d(m_iDataLen, m_fft_pin, m_fft_pout, FFTW_ESTIMATE);	//ʵ������������Hermite�Գ���ֻȡfs��һ��

	fftw_execute(p);

	//��ֵ
	for (int i = 0; i <(m_iDataLen / 2 - 1); i++)
	{
		m_fft_pabs[i] = pow(m_fft_pout[i][0], 2) + pow(m_fft_pout[i][1], 2);
		m_fft_pabs[i] = 10.0 * log10(m_fft_pabs[i]);	//log10(0)�����Ī������Ĵ���
	}


	// �ͷ���Դ
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