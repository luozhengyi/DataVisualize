#ifndef __MYMUTEX_H__
#define __MYMUTEX_H__
#include <queue>
/*
 * 其实应该是用线程ID，我使用的是对象ID，有点不入流，
 * 但是原理差不多，也能达到效果
 */


class CMyMutex
{

private:
	static std::queue<int> __obj_id_que;     //声明静态变量
	static int __obj_count;
private:
	const int m_obj_id;

public:
	CMyMutex();
	virtual ~CMyMutex();

public:
	void lock();
	void unlock();

};		//这个地方必须要加个 ；FUCK



#endif //__MYMUTEX_H__