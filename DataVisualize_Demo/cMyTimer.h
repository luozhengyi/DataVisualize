#ifndef __CMYTIMER_H__
#define __CMYTIMER_H__

#pragma once
#include <string>
#include "list"
#include "iostream"
#include "windows.h"
using namespace std;

/******************************* 定时器结构体 ****************************/
struct stTimer				//保存定时器信息的定时器
{
	unsigned timerID	;		//定时器ID
	unsigned timeElapse;		//定时器运行时间间隔
	unsigned timeLastRun;	//定时器上次运行时间
	int  iParam;				//预留参数
	string strParam;			//预留参数
	bool bDel;				//是否要被删除
	stTimer()				//构造函数,初始化
	{
		timerID=0;
		timeElapse=0;
		timeLastRun=0;
		iParam=0;
		strParam="";
		bDel=false;
	}
};
/************************************************************************/

/**************************** 类型重定义 *********************************/
typedef std::list<stTimer>				ListTimer;	
typedef std::list<stTimer>::iterator	  itListTimer;	
/************************************************************************/

/************************* 定时器类：cMyTimer ************************/
class cMyTimer
{
public:
	cMyTimer();
	virtual ~cMyTimer();
public:
	//添加定时器，启动定时器线程:timerID(定时器ID)、timeElapse(定时器时间间隔)、3，4(预留参数)
	void AddTimer(unsigned timerID,unsigned timeElapse,int iParam=0,string strParam="");	
	//定时器控制函数：判断、控制定时器的运行，被定时器线程回调函数CallBack_TimerProc()调用
	bool TimerCtrlRun();
public:
	//定时器响应函数，被定时器控制函数TimerCtrlRun()调用；
	//子类要继承、重载；如果返回0，则执行完这次后销毁，否则继续;也可以根据返回值修改间隔时间
	//Ontimer()内部应该判断timerID，不同的定时器ID执行不同的响应。
	virtual unsigned OnTimer(unsigned timerID,int iParam,string strParam)=0;
public:
	//检测、删除bDel状态为TRUE的定时器
	void CheckDelTimer();
	//根据timerID删除定时器
	void DeletTimer(unsigned timerID);


private:
	ListTimer m_listTimer;	//列表用来保存所有的定时器

};
/*********************************************************************/

#endif  //__CMYTIMER_H__