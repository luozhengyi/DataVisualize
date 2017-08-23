#include "stdafx.h"
#include "cMyTimer.h"
#include "process.h"		//多线程需要的头文件

/*******************************全局函数*********************************/
void CallBack_TimerProc(void* p)	//定时器线程的回调函数
{//不停的运行定时器控制函数控制定时器运行
	cMyTimer* pTimer=(cMyTimer*)p;	//获取当前cMyTimer的类对象指针
	if(pTimer==NULL)
	{
		return;
	}
	while(1)		//不停的运行定时器控制函数
	{
		//检测、删除bDel状态为TRUE的定时器
		pTimer->CheckDelTimer();
		if(!pTimer->TimerCtrlRun())	//如果定时器运行失败就退出线程
		{
			_endthread();//删除线程	
			return;
		}
		Sleep(20);		//睡眠20ms
	}

}
/************************************************************************/


/****************************cMyTimer成员函数******************************/
cMyTimer::cMyTimer()
{

}

cMyTimer::~cMyTimer()
{

}

void cMyTimer::AddTimer(unsigned timerID,unsigned timeElapse,int iParam,string strParam)
{//添加定时器，启动定时器线程

	if(timeElapse==0)
	{//如果定时器时间间隔为0，直接返回
		return;
	}

	//通过传入参数，构造一个定时器数据
	stTimer stTimerTemp;		//定时器结构体
	stTimerTemp.timerID=timerID;
	stTimerTemp.timeElapse=timeElapse;
	stTimerTemp.iParam=iParam;
	stTimerTemp.strParam=strParam;
	stTimerTemp.timeLastRun=::GetTickCount();	//当前时间(相对的,不是绝对的哦)

	//将构造的定时器加入到定时器列表中
	m_listTimer.push_back(stTimerTemp);
	
	//如果是第一个定时器就创建一个新的线程，其它的都跟第一个共一个线程，无需新建线程。
	if(m_listTimer.size()==1)
	{
		//第一参数：是线程的回调函数，第三个参数：是回调函数的参数
		_beginthread(CallBack_TimerProc,0,this); //创建定时器线程以用来运行定时器
	}
}

bool cMyTimer::TimerCtrlRun()
{//判断、控制定时器的运行
	if(m_listTimer.size()==0)
	{//定时器列表为空，即没有定时器时
		return false;
	}

	//判断、运行定时器。
	unsigned timeNow=GetTickCount();		//获取当前时间
	itListTimer it=m_listTimer.begin();	//定义定时器列表迭代器
	for( ; it!=m_listTimer.end();it++)	//遍历每个定时器，
	{
		//比较定时器上次运行的时间和当前时间差是否大于等于timeElapse
		if((timeNow-it->timeLastRun) >= it->timeElapse)
		{//如果大于，就运行定时器；如果小于就不运行。
			it->timeLastRun = timeNow;

			unsigned ret=OnTimer(it->timerID,it->iParam,it->strParam);
			//如果OnTimer()返回0，执行完这次后销毁该定时器。
			if(ret==0)
			{
				//改变定时器的销毁状态为true
				it->bDel=true;
			}
			/*else if(ret>1000)
			{//根据定时器返回值修改定时器间隔
				it->timeElapse=ret;
			}*/
		}
	}
	return true;
}

void cMyTimer::CheckDelTimer()
{//检测、删除bDel状态为TRUE的定时器
	itListTimer it=m_listTimer.begin();
	for( ;it!=m_listTimer.end(); )
	{
		if(it->bDel)
		{
			it=m_listTimer.erase(it);//删除定时器
			continue;
		}
		it++;
	}
}

void cMyTimer::DeletTimer(unsigned timerID)
{
	itListTimer it=m_listTimer.begin();
	for( ;it!=m_listTimer.end(); it++)
	{
		if(it->timerID==timerID)
		{
			it->bDel=true;
			return;
		}
	}
}
/************************************************************************/