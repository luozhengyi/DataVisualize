#include "stdafx.h"
#include "DSP.h"
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "Plot.h"

CDSP::CDSP(CPlot* pPlot) :m_plot(pPlot), m_dataLen(0)
{
	m_pIData = nullptr;
	m_pQData = nullptr;

	m_pISymbols = nullptr;
	m_pQSymbols = nullptr;
}

CDSP::~CDSP()
{
	if (m_pQData)
		delete[] m_pQData;
	if (m_pIData)
		delete[] m_pIData;

	if (m_pISymbols != nullptr)
		delete[] m_pISymbols;
	if (m_pQSymbols != nullptr)
		delete[] m_pQSymbols;
}

bool CDSP::QAM_Gen(double fs, double fd, double numSymbols)
{
	int oversampling = fs / fd;					//��������
	int iDataLen = numSymbols * oversampling;	//I��Q·���ݳ���
	int Ref = 1;								//�ο���ѹ
	double R = 0.5;								//����ϵ��

	//�����ڴ�
	double *pIData = new double[iDataLen]();	//��������ڴ��ʼ��Ϊ0
	double *pQData = new double[iDataLen]();
	if (pIData == NULL || pQData == NULL)
		return false;
	memset(pIData, 0.0, iDataLen*sizeof(double));
	memset(pQData, 0.0, iDataLen*sizeof(double));

	//�������������ɲ�ֵ��
	srand((unsigned)time(NULL));	//��ʼ�����������
	for (int i = 0, index = (i * oversampling); index < iDataLen; i++, index = (i * oversampling))
	{
		pIData[index] = rand() % 4 - 1.5;	//����-1.5 -0.5 0.5 1.5�������
		pQData[index] = rand() % 4 - 1.5;	//����-1.5 -0.5 0.5 1.5�������
	}
	

	//��ͨ�˲�����ֹƵ��(fd =1MHZ)����ֹƵ��̫��̫С����һ���Ǻ���,�������ԣ�
	double  fpass = (fd - 0.2e6) - 0.05e6,
			apass = -3,
			fstop = (fd - 0.2e6) + 0.05e6,
			astop = -20;		//-10dbҪ��-20db�˲�Ч���ã����Ƕ��ź�ȴ����һ���Ǻ���
	CFIR fir(FILTER_LOWPASS, WINDOW_BLACKMAN);
	fir.setParams( fs, fpass, apass, fstop, astop );
	fir.design();
	//fir.dispInfo();
	std::vector<double>	coefs = fir.getCoefs();
	//FilterIR(coefs);		//��λ�����Ӧ
	FilterFilt(pIData, iDataLen, coefs);
	FilterFilt(pQData, iDataLen, coefs);

	//�������ݵ��ļ�
	SaveToFile(pIData, pQData, iDataLen);

	//�����������
	coefs = RCosine(fs, fd, R);
	FilterIR(coefs);
	FilterFilt(pIData, iDataLen, coefs);
	FilterFilt(pQData, iDataLen, coefs);
	//�������ݵ��ļ�
	SaveToFile(pIData, pQData, iDataLen);

	//�������źŵ��Ƶ���Ƶ��
	double fc = 20e6;		//����һ��̫�����г��������
	for (int i = 0; i < iDataLen; i++)
	{
		/*pIData[i] *= cos(TWOPI*fc / fs*i);
		pQData[i] *= -sin(TWOPI*fc / fs*i);*/

		//matlab�±��1��ʼ,����ʵ����ʱ�ӣ��ͻ������ƫ
		pIData[i] *= cos(TWOPI*fc / fs*(i+1));
		pQData[i] *= -sin(TWOPI*fc / fs*(i + 1));

		pIData[i] += pQData[i];	//��I��Q·������ӵ�һ·��ŵ�I·���ڴ��ϡ�
	}
	

	//������д���ļ�
	//SaveToFile(pIData, iDataLen);

	//���
	//QAM_Demod(pIData, iDataLen, fs, fc, fd, R);

	
	
	//�˲�������
	//FilterIR(coefs);


	

	//�ͷ���Դ
	delete[] pIData;
	delete[] pQData;
	return true;
}

bool CDSP::QAM_Demod( double fc, double fd, double R)
{

	INT16* pData = m_plot->m_pDataY;
	int iDataLen = m_plot->m_iDataLen;
	double fs = m_plot->m_fs;

	int oversampling = fs / fd;					//��������
	int numSymbols = iDataLen / oversampling;	//������

	m_dataLen = iDataLen;
	m_symbolsLen = numSymbols;

	//�����ڴ�
	double *pIData = new double[iDataLen]();
	double *pQData = new double[iDataLen]();
	//double *pISymbols = new double[numSymbols]();
	//double *pQSymbols = new double[numSymbols]();
	m_pISymbols = new double[numSymbols]();
	m_pQSymbols = new double[numSymbols]();
	if (pIData == NULL || pQData == NULL || m_pISymbols == NULL || m_pQSymbols == NULL)
		return false;

	//��ͨ�˲����˳��ز��������ź�;��ͨ�˲������˵�̫�ݣ�����ᷢ����ƫ,�ȸ�������(FilterFilt()�������ƫ)
	double  //fstop1 = fc - fd * (1.0 + R) - 0.1e6,
			fstop1 = fc - fd * (1.0 + R) - 0.1,
			astop1 = -20,
			//fpass1 = fc - fd * (1.0 + R) + 0.1e6,
			//fpass2 = fc + fd * (1.0 + R) - 0.1e6,
			fpass1 = fc - fd * (1.0 + R) + 0.1,
			fpass2 = fc + fd * (1.0 + R) - 0.1,
			apass1 = -3,
			//fstop2 = fc + fd * (1.0 + R) + 0.1e6,
			fstop2 = fc + fd * (1.0 + R) + 0.1,
			astop2 = -20;
	CFIR fir(FILTER_BANDPASS, WINDOW_BLACKMAN);
	fir.setParams(fs, fstop1, astop1, fpass1, fpass2, apass1, fstop2, astop2);
	fir.design();
	//fir.dispInfo();
	std::vector<double>	coefs = fir.getCoefs();
	//FilterFilt(pData, iDataLen, coefs);
	//FiltFilt(pData, iDataLen, coefs);	//����ƫ�˲�
	//std::vector<double>	coefs;


	//��ɽ���������I��Q��·�ź�
	for (int i = 0; i < iDataLen; i++)
	{
		/*pIData[i] = 2.0 * pData[i] * cos(TWOPI * fc / fs * i);
		pQData[i] = 2.0 * pData[i] * -sin(TWOPI * fc / fs * i);*/

		//matlab���±��Ǵ�1��ʼ��,��matlab����Ա�
		pIData[i] = pData[i] * cos(TWOPI * fc / fs * (i + 1)) / (1<<15);
		pQData[i] = pData[i] * -sin(TWOPI * fc / fs * (i + 1)) / (1 << 15);
	}

	//�������ݵ��ļ�
	//SaveToFile(pIData, pQData, iDataLen);

	//ƥ���˲���Ҳ�ǵ�ͨ�˲���
	coefs=RCosine(fs, fd, R);
	FilterFilt(pIData, iDataLen, coefs);
	FilterFilt(pQData, iDataLen, coefs);

	//�������ݵ��ļ�
	//SaveToFile(pIData, pQData, iDataLen);

	

	//Ѱ����Ѳ�����,һ�����������ڲ�����oversampling���㣬��ÿ������Ϊ��������㣬���㹦�ʡ�
	int start = 0;		//��������ʼ�±�
	double *pPower = new double[oversampling]();	//���湦��
	if (pPower != NULL)
		for (int i = 0; i < oversampling; i++)
		{
			int index = 0;
			for (int j = 0; j < numSymbols/2; j++)		//���Բ���ȡȫ���ķ���
			{
				index = i + j*oversampling;
				pPower[i] += (pow(pIData[index],2) + pow(pQData[index],2));		//�þ���ֵ����ƽ����һ����
			}
				
		}
	start = std::max_element(pPower, pPower + oversampling ) - pPower;	//Ѱ�ҹ������ֵ������,oversampling����-1

	//������Ѳ�������г���
	for (int i = 0, index = 0; i < numSymbols && index < iDataLen; ++i, index = start + i*oversampling)
	{
		m_pISymbols[i] = pIData[index];
		m_pQSymbols[i] = pQData[index];
	}


	//����EVM


	//�������ݵ��ļ�
	//SaveToFile(pISymbols, pQSymbols, numSymbols);

	//�ͷ���Դ
	delete[] pIData;
	delete[] pQData;
	//delete[] pISymbols;
	//delete[] pQSymbols;
	delete[] pPower;
	return true;

}


std::vector<double> CDSP::FastConv(double* xn, int xn_len, double* hn, int hn_len)
{
	//xn_len >= hn_len
	if (xn_len < hn_len)
		return std::vector<double> (0);

	int iLen = xn_len + hn_len - 1;

	//���·����ڴ�
	//if (iLen % 2 != 0)	//fft�任��Ҫ��2�ı���(������ҪŶ)
	//iLen++;
	double *yn = (double *)fftw_malloc(sizeof(double) * iLen);
	std::vector<double> ve_yn(iLen, 0.0);
	double *xn_new = (double *)fftw_malloc(sizeof(double) * iLen);
	double *hn_new = (double *)fftw_malloc(sizeof(double) * iLen);
	fftw_complex *XK = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * iLen);
	fftw_complex *HK = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * iLen);
	fftw_complex *YK = NULL;

	//�ڴ濽��
	memmove(xn_new, xn, xn_len*sizeof(double));
	memmove(hn_new, hn, hn_len*sizeof(double));

	//����fft�任ll
	fftw_plan p1, p2;
	p1 = fftw_plan_dft_r2c_1d(iLen, xn_new, XK, FFTW_ESTIMATE);	//ʵ������������Hermite�Գ���ֻȡfs��һ��
	p2 = fftw_plan_dft_r2c_1d(iLen, hn_new, HK, FFTW_ESTIMATE);
	fftw_execute(p1);
	fftw_execute(p2);
	//����ʵ����FFT�Ĺ���Գ�����FFT����ĺ�һ��ֵ
	for (int i = iLen / 2 + 1; i < iLen; i++)
	{
		XK[i][0] = XK[iLen - i][0];
		XK[i][1] = -XK[iLen - i][1];

		HK[i][0] = HK[iLen - i][0];
		HK[i][1] = -HK[iLen - i][1];
	}

	//����ifft�任
	fftw_plan p3;
	YK = XK;
	double Re = 0.0;
	double Im = 0.0;
	for (int i = 0; i < iLen; i++)
	{
		Re = 0.0;
		Im = 0.0;
		Re = XK[i][0] * HK[i][0] - XK[i][1] * HK[i][1];
		Im = XK[i][0] * HK[i][1] + XK[i][1] * HK[i][0];
		YK[i][0] = Re / iLen;
		YK[i][1] = Im / iLen;
	}
	p3 = fftw_plan_dft_c2r_1d(iLen, YK, yn, FFTW_ESTIMATE);
	fftw_execute(p3);
	for (int i = 0; i < iLen; i++)	//�����鸳ֵ�� vector
	{
		ve_yn[i] = yn[i];
	}

	// �ͷ���Դ
	fftw_destroy_plan(p1);
	fftw_destroy_plan(p2);
	fftw_destroy_plan(p3);
	fftw_free(HK);
	fftw_free(XK);
	fftw_free(xn_new);
	fftw_free(hn_new);
	fftw_free(yn);

	return ve_yn;
}

template<class T>
std::vector<T> CDSP::Conv(T* xn, int xn_len, T* hn, int hn_len)
{
	//xn_len >= hn_len
	if (xn_len < hn_len)
		return std::vector<T>(0);


	int iLen = xn_len + hn_len - 1;
	std::vector<T> yn(iLen, 0.0);

	for (int i = 0; i < iLen; ++i)
	{
		yn[i] = 0;
		if (i <= hn_len-1)
			for (int j = 0; j <= i; ++j)
				yn[i] += hn[j] * xn[i - j];
		else if (i <= xn_len-1)
			for (int j = 0; j <= hn_len-1; ++j)
				yn[i] += hn[j] * xn[i - j];
		else
			for (int j = i - xn_len+1 ; j <= hn_len-1; ++j)
				yn[i] += hn[j] * xn[i - j];
	}
	return yn;
}

void CDSP::FilterFilt(double *pData, int iDataLen, std::vector<double>& filter)
{
	std::vector<double> result;

	//vector to Array
	double *pfilter = new double[filter.size()]();
	for (unsigned i = 0; i < filter.size(); i++)
		pfilter[i] = filter[i];
	
	//�˲�(�����
	result = FastConv(pData, iDataLen, pfilter, filter.size());

	//�˲������ֵ��ԭʼ�����ڴ�
	for (int i = 0; i < iDataLen; i++)
		pData[i] = result[i];
	//
	/*int index = 0;
	index = (filter.size()-1) / 2;
	for (int i = index; i < index+iDataLen; i++)
	{
		pData[i - index] = result[i];
	}*/
		

	//�ͷ���Դ
	delete[] pfilter;
}

void CDSP::FiltFilt(double *pData, int iDataLen, std::vector<double>& filter)
{
	//�ж������Ƿ���Ч
	int iFilterLen = filter.size();
	int iNewDataLen = 2*iFilterLen + iDataLen;	//����2�ƺ�Ҳ����
	if (iFilterLen <= 0 || pData  == NULL)
		return;

	//�ı�ԭʼ���ݳ��ȣ������ڴ�
	double *pNewData = new double[iNewDataLen]();
	memcpy(pNewData, pData, iDataLen*sizeof(double));

	//��һ���˲�(�����
	FilterFilt(pNewData,iNewDataLen,filter);
	//��һ��ת
	for (int i = 0; i < (iNewDataLen-1)/2; i++)
	{
		double temp = pNewData[i];
		pNewData[i] = pNewData[iNewDataLen - 1 - i];
		pNewData[iNewDataLen - 1 - i] = temp;
	}
	//�ڶ����˲�
	FilterFilt(pNewData, iNewDataLen, filter);
	//�ڶ���ת
	for (int i = 0; i < (iNewDataLen - 1) / 2; i++)
	{
		double temp = pNewData[i];
		pNewData[i] = pNewData[iNewDataLen - 1 - i];
		pNewData[iNewDataLen - 1 - i] = temp;
	}


	//�˲������ֵ��ԭʼ�����ڴ�
	memcpy(pData, pNewData, iDataLen*sizeof(double));

	//�ͷ���Դ
	delete[] pNewData;
}

void CDSP::SaveToFile(double *pData, int iDataLen)
{
	if (pData == NULL)	//�ж�����ָ���Ƿ���Ч
		return;
	std::fstream file1;
	file1.open("E:\\iqtools_2015_02_07\\iqtools\\vc_DATA.bin", std::ios::out | std::ios::binary);
	if (!file1)
		return;

	double minData = *(std::min_element(pData, pData + iDataLen - 1));
	double maxData = *(std::max_element(pData, pData + iDataLen - 1));
	double scale = abs(maxData) > abs(minData) ? abs(maxData) : abs(minData);

	short data = 0;
	char ch_data[2];
	for (int i = 0; i < iDataLen; i++)
	{
		data = 0;
		memset(ch_data, 0, 2);
		//��һ���ᵼ����matlab�л������ķ��Ȼ���һ��������ϵ
		//���Գ���10����20db
		data = (short)(pData[i] / scale * ((1 << 15) - 1));	//pData[i] / scaleһ������-1��1֮��ķ�Χ����Ȼ������ˡ�
		memcpy(ch_data, &data, 2);
		file1.write(ch_data, 2);
		
	}

	file1.close();
}

void CDSP::SaveToFile(double *pIData, double *pQData, int iDataLen)
{
	if (pIData == NULL || pQData == NULL)	//�ж�����ָ���Ƿ���Ч
		return;
	std::fstream file1;
	file1.open("C:\\Users\\machenike\\Desktop\\16QAM��������\\Lab_M_file\\vc_IQDATA.bin", std::ios::out | std::ios::binary);
	if (!file1)
		return;

	double minIData = *(std::min_element(pIData, pIData + iDataLen - 1));
	double maxIData = *(std::max_element(pIData, pIData + iDataLen - 1));
	double i_scale = abs(maxIData) > abs(minIData) ? abs(maxIData) : abs(minIData);

	double minQData = *(std::min_element(pQData, pQData + iDataLen - 1));
	double maxQData = *(std::max_element(pQData, pQData + iDataLen - 1));
	double q_scale = abs(maxQData) > abs(minQData) ? abs(maxQData) : abs(minQData);

	short data = 0;
	char ch_data[2];
	for (int i = 0; i < iDataLen; i++)
	{
		data = 0;
		memset(ch_data, 0, 2);
		//��һ���ᵼ����matlab�л������ķ��Ȼ���һ��������ϵ
		//���Գ���10����20db
		data = (short)(pIData[i] / i_scale * ((1 << 15) - 1));	//pData[i] / scaleһ������-1��1֮��ķ�Χ����Ȼ������ˡ�
		memcpy(ch_data, &data, 2);
		file1.write(ch_data, 2);

		data = 0;
		memset(ch_data, 0, 2);
		//��һ���ᵼ����matlab�л������ķ��Ȼ���һ��������ϵ
		//���Գ���10����20db
		data = (short)(pQData[i] / q_scale * ((1 << 15) - 1));	//pData[i] / scaleһ������-1��1֮��ķ�Χ����Ȼ������ˡ�
		memcpy(ch_data, &data, 2);
		file1.write(ch_data, 2);

	}

	file1.close();
}

void CDSP::FilterIR(std::vector<double>	coefs)
{
	//����ź�
	const int iDataLen = 40000;
	double dbData[iDataLen] = { 0.0 };
	dbData[0] = 1.0;

	FilterFilt(dbData, iDataLen, coefs);


	//�������ļ�
	SaveToFile(dbData, iDataLen);
}

std::vector<double> CDSP::RCosine(double fs, double fd, double R)
{
	if (R <= 0 || R>1)		//��֤����ϵ��
		return std::vector<double>();

	int order = (int)6.0 * fs / fd +1 ;	//�˲�����������ʽ���ϳ��ģ�
	double tau = (order - 1) / 2.0;		//�˲����ӳ�
	double Wc1 = PI*fd / fs;			//��ֹƵ��
	double t=0.0;
	std::vector<double> coefs(order);
	for (int i = 0; i < order; ++i)
	{
		t = i - tau;
		if (t == 0)
			coefs[i] = (1.0 - R) + 4.0*R / PI;
		else if (t == 1.0 / (4.0 * fd / fs * R) || t == -1.0 / (4.0 * fd / fs * R))		//��Ҫ����PI�����������⵼���жϳ���
			coefs[i] = R / sqrt(2.0) *((1 + 2 / PI)*sin(PI / (4.0 * R)) + (1 - 2 / PI)*cos(PI / (4.0 * R)));
		else
			coefs[i] = (4*R/PI*cos((1 + R)*Wc1*t) + sin((1 - R)*Wc1*t) / (Wc1*t)) / (1 - pow(4*R*t*Wc1/PI, 2));
	}

	return coefs;
}

std::vector<double> CDSP::GetDataFromFile()	//��ȡ��ͼ����
{
	std::vector<double> veData;
	char szDataPath[100] = "C:\\Users\\machenike\\Desktop\\cppData_transfer1.bin";
	int iDataLen = 0;
	do
	{
		if (strlen(szDataPath) == 0)
			break;
		std::fstream file1;
		file1.open(szDataPath, std::ios::in | std::ios::binary);
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
				veData.push_back(sData);
				num++;
				if (num == 199999)
					break;
			}
		}
		file1.close();
		
		return veData;	//��������

	} while (false);

	return std::vector<double>();

}

void CDSP::DemDataFromFile()
{
	std::vector<double> dbData = GetDataFromFile();
	int iDataLen = dbData.size();
	if (iDataLen == 0)
		return;
	double *pData = new double[dbData.size()]();
	if (pData == NULL)
		return;
	for (int i = 0; i < iDataLen; i++)
	{
		pData[i] = dbData[i]/(1<<15);
	}

	//QAM_Demod(pData, iDataLen, 100e6, 6.5e6, 1e6);

	delete[] pData;
}

int CDSP::gcd(int num, int den){	//շת������������Լ��
	// num / den = quo ....rem
	int iRet = 0;
	do
	{
		if (num == 0 || den == 0)
			break;
		int quo = 0, rem = 0;
		quo = num / den;
		rem = num % den;
		while (rem != 0)
		{
			num = den;
			den = rem;
			quo = num / den;
			rem = num % den;
		}

		iRet = den;
	} while (0);

	return iRet;
}
