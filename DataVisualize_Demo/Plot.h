#ifndef __CPLOT_H__
#define __CPLOT_H__
#include "cMyTimer.h"
#include <vector>

//fftw引用
#include "fftw\\fftw3.h"
#pragma comment(lib, "fftw\\libfftw3-3.lib") // double版本


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

class CPlot:public cMyTimer
{
#define DRAW_TIMER 50		//绘制定时器ID
#define NAME_LEN 200

public:
	CPlot();
	~CPlot();
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
private:
	void SetRect();	//设置矩形区域，由Draw和OnReshape调用
public:
	void SetBGColor(GLColor ClrBG);
	void SetLineColor(GLColor ClrLine);
public:
	//	设置坐标轴的范围
	void SetXMinAxis(float xmin){ m_xmin = xmin; }
	void SetXMaxAxis(float xmax){ m_xmax = xmax; }
	void SetYMinAxis(float ymin){ m_ymin = ymin; }
	void SetYMaxAxis(float ymax){ m_ymax = ymax; }
	void SetAxis(float xmin, float xmax, float ymin, float ymax);//	设置坐标轴的范围
	float GetXMinAxis(){ return m_xmin; }
	float GetXMaxAxis(){ return m_xmax; }
	float GetYMinAxis(){ return m_ymin; }
	float GetYMaxAxis(){ return m_ymax; }

	
public:
	void GetData();					//调用下面两个函数
	bool GetYDataFromFile();		//从文件读取获取绘图数据
	bool GetXYDataFromFile();
	bool GetDataFromSock();			//从socket获取绘图数据
	void GetMaxMinValue();			//获取数据的最大值或最小值
	void GetDataPath();				//获取数据文件路径
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
	GLColor m_ClrWnd;		//窗口背景颜色
	GLColor m_ClrBG;		//绘图区背景色
	GLColor m_ClrAxis;		//坐标颜色
	GLColor m_ClrLine;		//画图线条的颜色
	GLColor m_ClrText;		//文字颜色
private:
	enDataSource m_enDataSource;	//数据来源
	enFigureType m_enFigureType;	//绘制的图形类型
	enDrawType m_enDrawType;		//绘制原始信号、频谱、星座图
private:
	char m_szDataPath[NAME_LEN];	//数据文件路径
	double *m_pData;				//指向绘图数据的指针
	int m_iDataLen;					//数据长度
	std::vector<double> m_veDataX;	//数据向量
	std::vector<double> m_veDataY;	//数据向量
private:
	//缩放操作改变刻度
	double m_xscale;		//x轴坐标刻度
	double m_yscale;		//y轴坐标刻度
private:
	//
	double m_xmin;	//x轴上最小值
	double m_xmax;	//x轴上最大值
	double m_ymin;	//y轴上最小值
	double m_ymax;	//y轴上最大值
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
	int m_fs;					//采样频率：单位MHZ
	double *m_fft_pin ;			//fft输入信号
	double *m_fft_pabs;			//fft输出信号的绝对值
	fftw_complex *m_fft_pout ;	//fft输出复信号
public:
	void SetSampleFre(int fs){ m_fs = fs; }		//设置信号采样率
	int GetSampleFre(){ return m_fs; }
};

#endif //__CPLOT_H__


