#ifndef __CPLOT_H__
#define __CPLOT_H__
#include "cMyTimer.h"
#include <vector>
#include <winSock2.h>
#include "myMutex.h"
#include "DSP.h"

//fftw����
#include "fftw\\fftw3.h"
#pragma comment(lib, "fftw\\libfftw3-3.lib") // double�汾

class CPlot;
extern const int g_sockFrameLen;

struct GLColor
{
	float R;		//red
	float G;		//green
	float B;		//blue
	float A;		//alpha
};

struct GLRect
{
	float x;
	float y;
	float width;
	float heigh;
};

struct GLPoint
{
	float x;
	float y;
};

struct stFigureClrInfo		//��ͼ����ɫ��Ϣ
{
	GLColor ClrWnd;			//���ڱ�����ɫ
	GLColor ClrBG;			//��ͼ������ɫ
	GLColor ClrAxis;		//������ɫ
	GLColor ClrLine;		//��ͼ��������ɫ
	GLColor ClrText;		//������ɫ��û����

	stFigureClrInfo(){
		//ClrWnd = { 0.51f, 0.51f, 0.51f, 0.0f };	//���屳����ɫ
		ClrWnd = { 1.0f, 1.0f, 1.0f, 0.0f };		//���屳����ɫ
		//ClrBG = { 0.0f, 0.0f, 0.0f, 0.0f };		//��ͼ��������ɫ
		ClrBG = { 0.45f, 0.45f, 0.45f, 0.0f };		//��ͼ��������ɫ
		ClrAxis = { 0.0f, 0.0f, 0.0f, 0.0f };		//�����ɫ
		ClrLine = { 1.0f, 0.0f, 0.0f, 0.0f };		//��ͼ��ɫ
		ClrText = { 1.0f, 1.0f, 1.0f, 0.0f };		//���ְ�ɫ

	};
	void SetBGColor(GLColor glClrBG){
		ClrBG = glClrBG;
	};
	void SetLineColor(GLColor glClrLine){
		ClrLine = glClrLine;
	};
};

struct stSockInfo
{
	SOCKET sockSrv;				//������socket
	SOCKET sockClient;			//�ͻ�socket
	unsigned short sockPort;	//������socket�˿ں�
	const int sockDataLen;		//�������ݳ���
	const int sockFrameLen;		//֡������λbyte

	stSockInfo():
	sockDataLen(g_sockFrameLen / sizeof(short)), sockFrameLen(g_sockFrameLen)
	{
		sockSrv = INVALID_SOCKET;
		sockClient = INVALID_SOCKET;
		sockPort = 8888;

	};

	~stSockInfo(){
		//���socket��Դ
		closesocket(sockSrv);
		WSACleanup();
	}
};

struct stAxisInfo	//Figure����������Ϣ
{

	//���Ų����ı�̶�
	float xscale;		//x������̶�
	float yscale;		//y������̶�

	//
	float xmin;	//x������Сֵ
	float xmax;	//x�������ֵ
	float ymin;	//y������Сֵ
	float ymax;	//y�������ֵ

	int iScaleCount;	//ͼ��Ŵ�Ĵ���

	stAxisInfo()
	{
		xmin = 0.0;
		xmax = 1.0;
		ymin = 0.0;
		ymax = 1.0;
		xscale = (xmax - xmin) / 10.0;
		yscale = (ymax - ymin) / 10.0;
		iScaleCount = 0;
	};

	//	����������ķ�Χ
	void SetXMinAxis(float fxmin){ xmin = fxmin; }
	void SetXMaxAxis(float fxmax){ xmax = fxmax; }
	void SetYMinAxis(float fymin){ ymin = fymin; }
	void SetYMaxAxis(float fymax){ ymax = fymax; }
	void SetAxis(float fxmin, float fxmax, float fymin, float fymax)//	����������ķ�Χ
	{
		xmin = fxmin;
		xmax = fxmax;
		ymin = fymin;
		ymax = fymax;
	}
	float GetXMinAxis(){ return xmin; }
	float GetXMaxAxis(){ return xmax; }
	float GetYMinAxis(){ return ymin; }
	float GetYMaxAxis(){ return ymax; }
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

enum enDrawStatus
{
	E_Draw_Fresh =0,				//ͼƬ����ʵʱˢ��
	E_Draw_Static = 1,				//ͼƬ���ݲ��仯
	E_Draw_Stop = 2					//��������ʱ�Ͳ�����
};

struct stFile
{
	bool bfile;				//�Ƿ�ɼ����ݱ�־λ
	std::wstring filePath;	//�ļ�·����·����
	int frameNum;			//���ݲɼ���֡��
	FILE* fp;				//�ļ�ָ��
	stFile() :bfile(false), frameNum(0), fp(nullptr){
	}
	void Set_bfile(bool flag){
		bfile = flag;
	}
	bool Get_bfile(){
		return bfile;
	}
	void Set_filePath(wstring& file){
		filePath = file;
	}
	std::wstring Get_filePath(){
		return filePath;
	}
	void Set_frameNum(int num){
		frameNum = num;
	}
	int Get_frameNum(){
		return frameNum;
	}
	FILE* Get_fp(){
		return fp;
	}
	bool Init_stFile(){
		bool bRet = false;
		do
		{
			if (fp != nullptr)
			{
				fclose(fp);
				fp = nullptr;
			}
			bfile = false;

			_wfopen_s(&fp, filePath.c_str(), _T("wb+"));
			if (fp == nullptr)
				break;

			bfile = true;
			bRet = true;
		} while (0);

		return bRet;
	}
	void StopRead(){
		bfile = false;
		fclose(fp);
		fp = nullptr;
		frameNum = 0;
		filePath.clear();
	}

};

class CPlot:public cMyTimer
{
#define DRAW_TIMER 10		//���ƶ�ʱ��ID
#define GETDATA_TIMER 20	//��ȡ����ʱ�䶨ʱ��ID
#define NAME_LEN 200

	friend class CDSP;
public:
	CPlot();
	virtual ~CPlot();
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
public:
	static void CPlot::SetDrawStatus(enDrawStatus drawStatus){
		m_enDrawStatus = drawStatus;
	}
	static enDrawStatus CPlot::GetDrawFlag(){
		return m_enDrawStatus;
	}
private:
	void SetFigRect();	//����figure����������Draw��OnReshape����
public:
	stFigureClrInfo* GetFigClrInfo(){ return &m_stFigClrInfo; };
	stAxisInfo* GetAxisInfo(){ return &m_stAxisInfo; };
public:
	static void SetDataSource(enDataSource dataSource){ m_enDataSource = dataSource; }
	void SetDrawType(enDrawType drawType){ m_enDrawType = drawType; }
	void SetFigureType(enFigureType figureType){ m_enFigureType = figureType; }

public:
	void GetData();					//����������������
	bool GetYDataFromFile();		//���ļ���ȡ��ȡ��ͼ����
	bool GetXYDataFromFile();
	bool GetDataFromSock();			//��socket��ȡ��ͼ����
	void GetMaxMinValue();			//��ȡ���ݵ����ֵ����Сֵ
	void GetSock();					//����socket
	static	void GetDataPath(wchar_t* wszPath);		//��ȡ�����ļ�·��
	static stFile& GetstFile(){		//��ȡ���ݲɼ��ṹ��
		return m_stFile; 
	};				
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
	stFigureClrInfo m_stFigClrInfo;	//��ͼ����ɫ��Ϣ
	stSockInfo m_stSockInfo;		//Socket ��Ϣ
	stAxisInfo m_stAxisInfo;		//figure��������Ϣ
private:
	enFigureType m_enFigureType;	//���Ƶ�ͼ������
	enDrawType m_enDrawType;		//����ԭʼ�źš�Ƶ�ס�����ͼ
private:
	static int objNum;						//��ʵ���������
	static enDataSource m_enDataSource;		//������Դ
	static wchar_t* m_szDataPath;			//�����ļ�·��
	static int m_iDataLen;					//���ݳ���
	static INT16* m_pDataX;					//��������
	static INT16* m_pDataY;					//��������
	static enDrawStatus m_enDrawStatus;		//true����ʾ����ʵʱ���ݣ�false���෴
	static stFile m_stFile;					//���ݲɼ���Ϣ�ṹ��

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
	static float m_fs;					//����Ƶ�ʣ���λMHZ
	static double *m_fft_pin;			//fft�����ź�
	static double *m_fft_pabs;			//fft����źŵľ���ֵ
	static fftw_complex *m_fft_pout;	//fft������ź�
public:
	void SetSampleFre(int fs){ m_fs = fs; };		//�����źŲ�����
	int GetSampleFre(){ return m_fs; };
public:
	CDSP* GetDspObj(){
		return m_dsp;
	}
private:
	CDSP* m_dsp;

	
};

#endif //__CPLOT_H__


