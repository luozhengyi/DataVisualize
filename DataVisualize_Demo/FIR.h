#ifndef __FIR_H__
#define __FIR_H__
#include "DFD.h"
#include <vector>

enum enWindType
{
	WINDOW_RECT		= 0,
	WINDOW_BARTLETT = 1,
	WINDOW_HANNING	= 2,
	WINDOW_HAMMING	= 3,
	WINDOW_BLACKMAN = 4,
	WINDOW_GAUSS	= 5,
	WINDOW_KAISER	= 6,
};

const int		MAXTERM = 20;					//����kaiser�˲���

/*************************����������**************************************/

template<typename Type> std::vector<Type> window(const enWindType&, int, Type);
template<typename Type> std::vector<Type> window(const enWindType&, int, Type, Type);

template<typename Type> std::vector<Type> kaiser(int, Type, Type);
template<typename Type> std::vector<Type> gauss(int, Type, Type);

template<typename Type> Type I0(Type alpha);



class CFIR : public CDFD
{
public:
	CFIR(const enFiltType &select, const enWindType &win);
	CFIR(const enFiltType &select, const enWindType &win, double a);
	virtual ~CFIR();

public:
	void    design();						//��ơ������˲���(ϵ��)
	void    dispInfo() const;				//�����ʾ��Ƶ��˲������
	std::vector<double> getCoefs();			//��ȡ�˲�����ϵ��

private:
	void    orderEst();					//�����˲����ĳ���(����)  ע���ϸ��˵FIR�˲����Ľ���=��ϵ������-1���˴���Ϊ��ͬ��Ӱ��
	void    idealCoef();				//��������FIR�˲���ϵ��
	void    calcCoef();					//����ʵ��FIR�˲���ϵ��(����ļӴ�)
	double  frqeResp(double freq);		//����Ƶ����Ӧ��������ɢʱ�丵��Ҷ�任
	void    calcGain();					//��������
	bool    isSatisfy();				//�ж���Ƶ��˲����Ƿ����ָ��

	enWindType  m_windType;				// ��������
	std::vector<double>  m_wind;		// window function
	std::vector<double>	m_coefs;		// coefficients
	std::vector<double>	m_edgeGain;		// �߽�Ƶ�ʵ����棬�����ж���Ƶ��˲����Ƿ����ָ��
	double  m_alpha;					// window parameter


	/* ����matlab������ӵ�һЩĿǰʹ�õĸ���Ĵ����� */
public:


};	// class CFIR



#endif	//__FIR_H__


