#ifndef __DFD_H__
#define __DFD_H__
#include <string>

const double    FREQ_MIN = 1.0E-3;      // ��СƵ��
const double    FREQ_MAX = 1.0E12;      // ���Ƶ��
const double    GAIN_PASS = -1.0E-2;    // ���ͨ������
const double    GAIN_TRAN = -3.0103;    // min pb, max sb gain
const double    GAIN_STOP = -1.0E02;    // ��С�������

const double	EPS = 2.220446049250313e-016;	//��������С�ļ����Ҳ���Ǹ������ľ���

const double	PI = 3.141592653589793;			//Բ����pi
const double	TWOPI = 6.283185307179586;


enum enFiltType
{
	FILTER_LOWPASS	= 0,
	FILTER_HIGHPASS = 1,
	FILTER_BANDPASS = 2,
	FILTER_BANDSTOP = 3,
};


class CDFD
{
public:
	CDFD(const enFiltType &select);
	virtual ~CDFD();
public:
	//�����˲��������������ڵ�ͨ�͸�ͨ
	void setParams(double fs, double f1, double a1,double f2, double a2);
	//�����˲��������������ڴ�ͨ�ʹ���
	void setParams(double fs, double f1, double a1, double f2,double f3, double a2, double f4, double a3);

	//����˲���
	virtual void design() = 0;
	virtual void dispInfo() const = 0;

protected:

	//��λHZ
	double  fsamp;              // ����Ƶ��
	double  wpass1, wpass2;     // ͨ���߽�Ƶ��
	double  wstop1, wstop2;     // ����߽�Ƶ��

	//��λdB
	double  apass1, apass2;     // ͨ������
	double  astop1, astop2;     // �������

	enFiltType  m_filtType;     // �˲���������
	int     order;              // �˲����ĳ���

	//��ȡ����Ĳ���
	inline double getValue(double x, double min, double max);

};

#endif	//__DFD_H__


