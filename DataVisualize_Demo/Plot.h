#ifndef __CPLOT_H__
#define __CPLOT_H__
#include "cMyTimer.h"
#include <vector>

//fftw����
#include "fftw\\fftw3.h"
#pragma comment(lib, "fftw\\libfftw3-3.lib") // double�汾


struct GLColor
{
	float R;
	float G;
	float B;
	float A;
};

struct GLRect
{
	float x;
	float y;
	float width;
	float height;
};

struct GLPoint
{
	float x;
	float y;
};

enum enFigureType
{
	E_FigureType_Normal=1,		//��ͨ����
	E_FigureType_Point=2,		//ɢ״��
	E_FigureType_Column=3,		//��״ͼ
};

enum enDataSource
{
	en_From_File = 0,	//���ļ���ȡ����
	en_From_Sock = 1,	//��socket��������
	en_From_Prog = 2,	//�����Լ���������
};

enum enDrawType
{
	en_Draw_Orignal = 0,			//����ԭʼ�ź�
	en_Draw_Spectrum = 1,			//����Ƶ��
	en_Draw_Constellation = 2		//��������ͼ
};

class CPlot:public cMyTimer
{
#define DRAW_TIMER 50		//���ƶ�ʱ��ID
#define NAME_LEN 200

public:
	CPlot();
	~CPlot();
public:
	
	virtual unsigned OnTimer(unsigned timerID, int iParam, string strParam);
	bool Init(HWND hWnd);
private:
	void InitGL();				//��Init()����
	void SetupPixelFormat();	//��Init()����
	void InitText();			//��������
public:
	void Draw();
	void DrawAxis();
	void DrawData();
	void DrawBG();
	void DrawText();
private:
	void SetRect();	//���þ���������Draw��OnReshape����
public:
	void SetBGColor(GLColor ClrBG);
	void SetLineColor(GLColor ClrLine);
public:
	//	����������ķ�Χ
	void SetXMinAxis(float xmin){ m_xmin = xmin; }
	void SetXMaxAxis(float xmax){ m_xmax = xmax; }
	void SetYMinAxis(float ymin){ m_ymin = ymin; }
	void SetYMaxAxis(float ymax){ m_ymax = ymax; }
	void SetAxis(float xmin, float xmax, float ymin, float ymax);//	����������ķ�Χ
	float GetXMinAxis(){ return m_xmin; }
	float GetXMaxAxis(){ return m_xmax; }
	float GetYMinAxis(){ return m_ymin; }
	float GetYMaxAxis(){ return m_ymax; }

	
public:
	void GetData();					//����������������
	bool GetYDataFromFile();		//���ļ���ȡ��ȡ��ͼ����
	bool GetXYDataFromFile();
	bool GetDataFromSock();			//��socket��ȡ��ͼ����
	void GetMaxMinValue();			//��ȡ���ݵ����ֵ����Сֵ
	void GetDataPath();				//��ȡ�����ļ�·��
public:
	void OnReshape();	//������״�ı���Ӧ����
	//���ʾ��Ƕ�OpenGL�����귶Χ(m_xmin,m_xmax,...)���б仯
	void OnTranslate(const tagPOINT ptPre, const tagPOINT ptNow);	//ƽ��ͼ��,ͼ����������ƶ�
	void OnScale(float fTime);			//����ͼ��,�������޵���С
public:
	bool IsinRect(const tagPOINT pt);		//�ж�����Ƿ��ڻ�ͼ������
	void ShowPosCoord(const tagPOINT pt);	//��ʾ�������ֵ
	GLPoint ScreentoOpenGLCoord(const tagPOINT Screen_pt);	//����Ļ��������ת��ΪOpenGL����
	tagPOINT OpenGLtoScreenCoord(const GLPoint OpenGL_pt);	//��OpenGL����ת��Ϊ��Ļ��������
public:
	void Set_m_bShowCoord(bool bRet){m_bShowCoord=bRet;} //�����Ƿ���ʾ���������
	void Set_tagPt(const tagPOINT pt){m_tagPt=pt;}		//��¼���λ��
public:
	void DestroyPlot();
private:
	HWND m_hWnd;
	HDC m_hDC;
	//HDC m_memDC;
	HGLRC m_hRC;
	//���ڿͻ�����Ϣ,����Ļ���½�Ϊԭ��
	RECT m_ClientRect;
	int m_iClientWidth;
	int m_iClientHeight;
	//��ͼ������Ϣ,����Ļ���½�Ϊԭ��(�м�)
	GLRect m_FigRect;

private:
	GLColor m_ClrWnd;		//���ڱ�����ɫ
	GLColor m_ClrBG;		//��ͼ������ɫ
	GLColor m_ClrAxis;		//������ɫ
	GLColor m_ClrLine;		//��ͼ��������ɫ
	GLColor m_ClrText;		//������ɫ
private:
	enDataSource m_enDataSource;	//������Դ
	enFigureType m_enFigureType;	//���Ƶ�ͼ������
	enDrawType m_enDrawType;		//����ԭʼ�źš�Ƶ�ס�����ͼ
private:
	char m_szDataPath[NAME_LEN];	//�����ļ�·��
	double *m_pData;				//ָ���ͼ���ݵ�ָ��
	int m_iDataLen;					//���ݳ���
	std::vector<double> m_veDataX;	//��������
	std::vector<double> m_veDataY;	//��������
private:
	//���Ų����ı�̶�
	double m_xscale;		//x������̶�
	double m_yscale;		//y������̶�
private:
	//
	double m_xmin;	//x������Сֵ
	double m_xmax;	//x�������ֵ
	double m_ymin;	//y������Сֵ
	double m_ymax;	//y�������ֵ
private:
	bool m_bShowCoord;	//���꣬����ʾ���������
	tagPOINT m_tagPt;	//��������
private:
	HGDIOBJ m_hOldFont;
	HGDIOBJ m_hNewFont;
private:
	int m_iScaleCount;	//ͼ��Ŵ�Ĵ���

	
/*****************************�źŴ�����******************************/
public:
	void fft();
	void DrawOrignal();
	void DrawSpectrum();		//�����ź�Ƶ��
	void DrawConstellation();	//��������ͼ
private:
	int m_fs;					//����Ƶ�ʣ���λMHZ
	double *m_fft_pin ;			//fft�����ź�
	double *m_fft_pabs;			//fft����źŵľ���ֵ
	fftw_complex *m_fft_pout ;	//fft������ź�
public:
	void SetSampleFre(int fs){ m_fs = fs; }		//�����źŲ�����
	int GetSampleFre(){ return m_fs; }
};

#endif //__CPLOT_H__


