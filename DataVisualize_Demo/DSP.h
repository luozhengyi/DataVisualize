#ifndef __DSP_H__
#define __DSP_H__
#include <vector>
#include "FIR.h"
//fftw����
#include "fftw\\fftw3.h"
//#pragma comment(lib, "fftw\\libfftw3-3.lib") // double�汾

class CPlot;

class CDSP
{
	friend class CPlot;

public:
	CDSP(CPlot* pPlot );
	virtual ~CDSP();

public:
	/**
	* @fn	bool QAM_Gen();
	* @brief	����16QAM�ź�.
	* @param	pIData:���ɵ�I·����	pQData:���ɵ�Q·����	fs:������	fd:������	numSymbols:������
	* @return	true if it succeeds,false if it fails
	*/
	bool QAM_Gen(double fs, double fd, double numSymbols);

	/**
	* @fn	bool QAM_Demod();
	* @brief	���16QAM�ź�.
	* @param	pData:I��Q��·�ϳ��ź�	iDataLen:���ݳ���	fs:������	fc:�ز�Ƶ��		fd:������	R:����ϵ��
	* @return	
	*/
	bool QAM_Demod(double fc, double fd, double R=0.5);
public:
	//xn�ĳ���Ҫ���ڵ���hn�ĳ��ȣ�
	template<class T>
	std::vector<T> Conv(T* xn, int xn_len, T* hn, int hn_len);
	//std::vector<double> Conv(double* xn, int xn_len, double* hn, int hn_len);		//���þ�����������Ծ��
	std::vector<double> FastConv(double* xn, int xn_len, double* hn, int hn_len);	//����fft����ѭ��(����)���
public:
	void FilterFilt(double *pData, int iDataLen, std::vector<double>& filter);		//ִ���˲�,������matlab��filter,ȡǰn�������������ƫ
	void FiltFilt(double *pData, int iDataLen, std::vector<double>& filter);		//�ȼ���matlab�е�filtfilt()����ƫ�˲���
public:
	void SaveToFile(double *pData, int iDataLen);					//�����ݱ��浽�ļ�
	void SaveToFile(double *pIData, double *pQData, int iDataLen);	//�����ݱ��浽�ļ�

public:
	std::vector<double> RCosine(double fs, double fd, double R);	//�������˲���
public:
	void FilterIR(std::vector<double> coefs);	//�˲�����λ�����Ӧ
public:
	std::vector<double> CDSP::GetDataFromFile();	//��ȡ����
	void DemDataFromFile();
public:
	int gcd(int num, int den);					//num: ���ӣ�  den: ��ĸ��
	void DDC(double sig, double fs, double fc);	//�����±�Ƶ(Digital Down Converter)

	


private:
	double *m_pIData;
	double *m_pQData;
	int m_dataLen;
	double *m_pISymbols;
	double *m_pQSymbols;
	int m_symbolsLen;
	
private:
	CPlot* m_plot;
};
#endif	//__DSP_H__


