#include "stdafx.h"
#include "myMutex.h"
#include "windows.h"



std::queue<int> CMyMutex::__obj_id_que;     //定义静态变量
int CMyMutex::__obj_count = 0;

CMyMutex::CMyMutex():
	m_obj_id(__obj_count+1)		//为每个对象分配唯一的ID
{
		__obj_count++;
}

CMyMutex::~CMyMutex()
{
	while (!__obj_id_que.empty())
		__obj_id_que.pop();
}

void CMyMutex::lock()
{
	__obj_id_que.push(m_obj_id);			//将当前对象ID压入队列
	while(__obj_id_que.front() != m_obj_id)		//如果当前对象ID不是处于队列的最上方
	{
		//线程挂起
		::Sleep(1);		//线程睡眠5ms，然后再去判断是否该它执行了
	}
}

void CMyMutex::unlock()
{
	if (!__obj_id_que.empty() && __obj_id_que.front() == m_obj_id)
		__obj_id_que.pop();		//当前对象所在线程执行完了，弹出对象ID
	
}

