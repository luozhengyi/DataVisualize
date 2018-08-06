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
	void CFFT::resize();	//�����ݵ���Ϊ2����������
	void CFFT::fft(CArray &data);		//fft���任
	void CFFT::ifft(CArray &data);		//fft���任
	void print();								//���

public:
	void SetSignal(CArray& data);	//�����ź�����
	CArray& GetSignal(void);		//��ȡ�ź�����
	size_t GetSignalSize();	//�źŵĴ�С
private:
	CArray& m_data;		//�ź�����
	size_t m_dataSize;	//�źŵĴ�С
private:
	enStatus m_enStatus;
};

#endif	// __FFT_H__


