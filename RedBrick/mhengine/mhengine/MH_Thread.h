/* 
 * This file is part of RedBrick.
 * Copyright (c) 2018 Link Information Systems Co., Ltd.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MH_THREAD_H
#define MH_THREAD_H

#include "MH_Engine.h"
#include "MH_Event.h"

class MHENGINE_API MH_Thread
{
private:
#ifdef WIN32
	HANDLE m_thread;
	
	static unsigned __stdcall stubMain(void * arg);
#else
	pthread_t m_thread;

	static void* stubMain(void * arg);
#endif

	MH_Event m_terminate;
	bool m_is_terminate;
	
	char m_tag[255];

protected:
	bool ThreadSleep(int msec);

	bool IsTerminate();

	virtual bool OnStart() { return true; }
	virtual void OnStop() {}
	virtual void OnMain() = 0;
	virtual void OnReqTerminate() {}

public:
	MH_Thread();
	virtual ~MH_Thread();
	
	virtual bool Start(const char* tag = NULL, int priority = 0);
	virtual void Stop();

	void SetTerminate();
	void WaitForTerminate();
};

#endif
