#ifndef __MYMUTEX_H__
#define __MYMUTEX_H__
#include <queue>
/*
 * ��ʵӦ�������߳�ID����ʹ�õ��Ƕ���ID���е㲻������
 * ����ԭ���࣬Ҳ�ܴﵽЧ��
 */


class CMyMutex
{

private:
	static std::queue<int> __obj_id_que;     //������̬����
	static int __obj_count;
private:
	const int m_obj_id;

public:
	CMyMutex();
	virtual ~CMyMutex();

public:
	void lock();
	void unlock();

};		//����ط�����Ҫ�Ӹ� ��FUCK



#endif //__MYMUTEX_H__