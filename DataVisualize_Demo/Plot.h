#ifndef __CPLOT_H__
#define __CPLOT_H__
#include "cMyTimer.h"
#include <vector>
#include <winSock2.h>
#include "myMutex.h"
#include "DSP.h"

//fftw引用
#include "fftw\\fftw3.h"
#pragma comment(lib, "fftw\\libfftw3-3.lib") // double版本

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

struct stFigureClrInfo		//绘图的颜色信息
{
	GLColor ClrWnd;			//窗口背景颜色
	GLColor ClrBG;			//绘图区背景色
	GLColor ClrAxis;		//坐标颜色
	GLColor ClrLine;		//画图线条的颜色
	GLColor ClrText;		//文字颜色，没用上

	stFigureClrInfo(){
		//ClrWnd = { 0.51f, 0.51f, 0.51f, 0.0f };	//窗体背景灰色
		ClrWnd = { 1.0f, 1.0f, 1.0f, 0.0f };		//窗体背景白色
		//ClrBG = { 0.0f, 0.0f, 0.0f, 0.0f };		//绘图区背景黑色
		ClrBG = { 0.45f, 0.45f, 0.45f, 0.0f };		//绘图区背景灰色
		ClrAxis = { 0.0f, 0.0f, 0.0f, 0.0f };		//坐标黑色
		ClrLine = { 1.0f, 0.0f, 0.0f, 0.0f };		//绘图红色
		ClrText = { 1.0f, 1.0f, 1.0f, 0.0f };		//文字白色

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
	SOCKET sockSrv;				//服务器socket
	SOCKET sockClient;			//客户socket
	unsigned short sockPort;	//服务器socket端口号
	const int sockDataLen;		//整形数据长度
	const int sockFrameLen;		//帧长：单位byte

	stSockInfo():
	sockDataLen(g_sockFrameLen / sizeof(short)), sockFrameLen(g_sockFrameLen)
	{
		sockSrv = INVALID_SOCKET;
		sockClient = INVALID_SOCKET;
		sockPort = 8888;

	};

	~stSockInfo(){
		//清除socket资源
		closesocket(sockSrv);
		WSACleanup();
	}
};

struct stAxisInfo	//Figure的坐标轴信息
{

	//缩放操作改变刻度
	float xscale;		//x轴坐标刻度
	float yscale;		//y轴坐标刻度

	//
	float xmin;	//x轴上最小值
	float xmax;	//x轴上最大值
	float ymin;	//y轴上最小值
	float ymax;	//y轴上最大值

	int iScaleCount;	//图像放大的次数

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

	//	设置坐标轴的范围
	void SetXMinAxis(float fxmin){ xmin = fxmin; }
	void SetXMaxAxis(float fxmax){ xmax = fxmax; }
	void SetYMinAxis(float fymin){ ymin = fymin; }
	void SetYMaxAxis(float fymax){ ymax = fymax; }
	void SetAxis(float fxmin, float fxmax, float fymin, float fymax)//	设置坐标轴的范围
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
	E_FigureType_Normal=1,		//普通连线
	E_FigureType_Point=2,		//散状点
	E_FigureType_Column=3,		//柱状图
};

enum enDataSource
{
	en_From_File = 0,	//从文件读取数据
	en_From_Sock = 1,	//从socket传输数据
	en_From_Prog = 2,	//程序自己生成数据
};

enum enDrawType
{
	en_Draw_Orignal = 0,			//绘制原始信号
	en_Draw_Spectrum = 1,			//绘制频谱
	en_Draw_Constellation = 2		//绘制星座图
};

enum enDrawStatus
{
	E_Draw_Fresh =0,				//图片数据实时刷新
	E_Draw_Static = 1,				//图片数据不变化
	E_Draw_Stop = 2					//窗口隐藏时就不画了
};

struct stFile
{
	bool bfile;				//是否采集数据标志位
	std::wstring filePath;	//文件路径和路径名
	int frameNum;			//数据采集的帧数
	FILE* fp;				//文件指针
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
#define DRAW_TIMER 10		//绘制定时器ID
#define GETDATA_TIMER 20	//获取数据时间定时器ID
#define NAME_LEN 200

	friend class CDSP;
public:
	CPlot();
	virtual ~CPlot();
public:
	
	virtual unsigned OnTimer(unsigned timerID, int iParam, string strParam);
	bool Init(HWND hWnd);
private:
	void InitGL();				//被Init()调用
	void SetupPixelFormat();	//被Init()调用
	void InitText();			//设置字体
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
	void SetFigRect();	//设置figure矩形区域，由Draw和OnReshape调用
public:
	stFigureClrInfo* GetFigClrInfo(){ return &m_stFigClrInfo; };
	stAxisInfo* GetAxisInfo(){ return &m_stAxisInfo; };
public:
	static void SetDataSource(enDataSource dataSource){ m_enDataSource = dataSource; }
	void SetDrawType(enDrawType drawType){ m_enDrawType = drawType; }
	void SetFigureType(enFigureType figureType){ m_enFigureType = figureType; }

public:
	void GetData();					//调用下面两个函数
	bool GetYDataFromFile();		//从文件读取获取绘图数据
	bool GetXYDataFromFile();
	bool GetDataFromSock();			//从socket获取绘图数据
	void GetMaxMinValue();			//获取数据的最大值或最小值
	void GetSock();					//开辟socket
	static	void GetDataPath(wchar_t* wszPath);		//获取数据文件路径
	static stFile& GetstFile(){		//获取数据采集结构体
		return m_stFile; 
	};				
public:
	void OnReshape();	//窗口形状改变响应函数
	//本质就是对OpenGL的坐标范围(m_xmin,m_xmax,...)进行变化
	void OnTranslate(const tagPOINT ptPre, const tagPOINT ptNow);	//平移图像,图像随着鼠标移动
	void OnScale(float fTime);			//缩放图像,不能无限的缩小
public:
	bool IsinRect(const tagPOINT pt);		//判断鼠标是否在绘图区域内
	void ShowPosCoord(const tagPOINT pt);	//显示点的坐标值
	GLPoint ScreentoOpenGLCoord(const tagPOINT Screen_pt);	//将屏幕像素坐标转换为OpenGL坐标
	tagPOINT OpenGLtoScreenCoord(const GLPoint OpenGL_pt);	//将OpenGL坐标转换为屏幕像素坐标
public:
	void Set_m_bShowCoord(bool bRet){m_bShowCoord=bRet;} //设置是否显示鼠标的坐标点
	void Set_tagPt(const tagPOINT pt){m_tagPt=pt;}		//记录鼠标位置
public:
	void DestroyPlot();
private:
	HWND m_hWnd;
	HDC m_hDC;
	//HDC m_memDC;
	HGLRC m_hRC;
	//窗口客户区信息,以屏幕左下角为原点
	RECT m_ClientRect;
	int m_iClientWidth;
	int m_iClientHeight;
	//绘图区域信息,以屏幕左下角为原点(切记)
	GLRect m_FigRect;

private:
	stFigureClrInfo m_stFigClrInfo;	//绘图的颜色信息
	stSockInfo m_stSockInfo;		//Socket 信息
	stAxisInfo m_stAxisInfo;		//figure坐标轴信息
private:
	enFigureType m_enFigureType;	//绘制的图形类型
	enDrawType m_enDrawType;		//绘制原始信号、频谱、星座图
private:
	static int objNum;						//类实例对象个数
	static enDataSource m_enDataSource;		//数据来源
	static wchar_t* m_szDataPath;			//数据文件路径
	static int m_iDataLen;					//数据长度
	static INT16* m_pDataX;					//数据向量
	static INT16* m_pDataY;					//数据向量
	static enDrawStatus m_enDrawStatus;		//true：表示绘制实时数据；false：相反
	static stFile m_stFile;					//数据采集信息结构体

private:
	bool m_bShowCoord;	//定标，即显示鼠标点的坐标
	tagPOINT m_tagPt;	//鼠标的坐标
private:
	HGDIOBJ m_hOldFont;
	HGDIOBJ m_hNewFont;
private:
	int m_iScaleCount;	//图像放大的次数

	
/*****************************信号处理部分******************************/
public:
	void fft();
	void DrawOrignal();
	void DrawSpectrum();		//绘制信号频谱
	void DrawConstellation();	//绘制星座图
private:
	static float m_fs;					//采样频率：单位MHZ
	static double *m_fft_pin;			//fft输入信号
	static double *m_fft_pabs;			//fft输出信号的绝对值
	static fftw_complex *m_fft_pout;	//fft输出复信号
public:
	void SetSampleFre(int fs){ m_fs = fs; };		//设置信号采样率
	int GetSampleFre(){ return m_fs; };
public:
	CDSP* GetDspObj(){
		return m_dsp;
	}
private:
	CDSP* m_dsp;

	
};

#endif //__CPLOT_H__


