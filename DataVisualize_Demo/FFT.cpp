#include "stdafx.h"
#include "FFT.h"


CFFT::CFFT(CArray& data)
	:m_data(data)
{
	
	m_dataSize = 0;
	m_enStatus = en_NOTRANS;
}

CFFT::~CFFT()
{
	if (m_data.size() != 0)
		m_data.free();
}

void CFFT::SetSignal(CArray& data)
{
	m_data = data;
	m_dataSize = m_data.size();
	resize();
}

CArray& CFFT::GetSignal(void)
{
	return m_data;
}

size_t CFFT::GetSignalSize()
{
	return m_dataSize ;
}

// Cooley¨CTukey FFT (in-place, divide-and-conquer)
// Higher memory requirements and redundancy although more intuitive
void CFFT::resize()
{
	size_t N1 = m_data.size();
	size_t N2 = N1;
	double power = log((double)N1) / log(2.0);
	if ((power - (int)power) > 0.000001)
	{
		N2 = 1 << ((int)power + 1);
		CArray xTemp = m_data;
		m_data.resize(N2);
		for (int i = 0; i < N1; i++)
			m_data[i] = xTemp[i];
		//m_data += xTemp;
		for (int i = N1; i < N2; i++)
			m_data[i] = m_data[i-N1];
		xTemp.free();
	}


}

void CFFT::fft(CArray &data)
{
	const size_t N = data.size();
	if (N <= 1) return;

	// divide
	CArray even = data[std::slice(0, N / 2, 2)];
	CArray  odd = data[std::slice(1, N / 2, 2)];

	// conquer
	fft(even);
	fft(odd);

	// combine
	for (size_t k = 0; k < N / 2; ++k)
	{
		Complex t = std::polar(1.0, -2 * PI * k / N) * odd[k];
		data[k] = even[k] + t;
		data[k + N / 2] = even[k] - t;
	}

	m_enStatus = en_FFT;
}

void CFFT::ifft(CArray &data)
{
	// conjugate the complex numbers
	data = data.apply(std::conj);

	// forward fft
	fft(data);

	// conjugate the complex numbers again
	data = data.apply(std::conj);

	// scale the numbers
	data /= data.size();

	m_enStatus = en_IFFT;
}

void CFFT::print()
{
	if (m_enStatus==en_FFT)
		std::cout << "fft" << std::endl;
	else if (m_enStatus == en_IFFT)
		std::cout << "ifft" << std::endl;
	else
		std::cout << "Original Signal" << std::endl;
	for (int i = 0; i < m_dataSize; ++i)
	{
		std::cout << m_data[i] << std::endl;
	}
}