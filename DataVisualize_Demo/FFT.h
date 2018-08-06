#ifndef __FFT_H__
#define __FFT_H__
#include <complex>
#include <iostream>
#include <valarray>

// refer:https://rosettacode.org/wiki/Fast_Fourier_transform

const double PI = 3.141592653589793238460;

typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;
enum enStatus
{
	en_NOTRANS = 0,
	en_FFT = 1,
	en_IFFT = 2
};

class CFFT
{
public:
	CFFT(CArray& data);
	~CFFT();
public:
	void CFFT::resize();	//将数据调整为2的整数次幂
	void CFFT::fft(CArray &data);		//fft正变换
	void CFFT::ifft(CArray &data);		//fft反变换
	void print();								//输出

public:
	void SetSignal(CArray& data);	//设置信号数据
	CArray& GetSignal(void);		//获取信号数据
	size_t GetSignalSize();	//信号的大小
private:
	CArray& m_data;		//信号数据
	size_t m_dataSize;	//信号的大小
private:
	enStatus m_enStatus;
};

#endif	// __FFT_H__


