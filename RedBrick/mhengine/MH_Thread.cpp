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

#include "stdafx.h"
#include "mhengine/MH_Thread.h"

MH_Thread::MH_Thread()
#ifdef WIN32
	: m_thread(NULL)
#else
	: m_thread(0)
#endif
	, m_terminate()
	, m_is_terminate(true)
	, m_tag()
{
}

MH_Thread::~MH_Thread()
{
}

bool MH_Thread::Start(const char* tag, int priority)
{
#ifdef WIN32
	if (m_thread != NULL)
#else
	if (m_thread != 0)
#endif
	{
		return false;
	}

	if(tag != NULL)
	{
		strcpy(m_tag, tag);
	}
	else
	{
		strcpy(m_tag, "");
	}

	if (!m_terminate.Create(true))
	{
		return false;
	}

	if (!OnStart())
	{
		m_terminate.Close();
		return false;
	}

	m_is_terminate = false;

#ifdef WIN32
	unsigned thread_id = 0;
	m_thread = (HANDLE)_beginthreadex(
		NULL,
		0,
		MH_Thread::stubMain,
		this,
		0,
		&thread_id
		);
	if (m_thread == NULL)
	{
		OnStop();

		m_is_terminate = true;
		m_terminate.Close();
		return false;
	}

	if (priority > 0)
	{
		SetThreadPriority(m_thread, THREAD_PRIORITY_HIGHEST);
	}

#else
	int ret;

	pthread_attr_t attr;
	ret = pthread_attr_init(&attr);
    if(ret != 0)
    {
    	return false;
    }

	if(priority > 0)
	{
#ifdef ANDROID_OS
		ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	    if(ret != 0)
	    {
	    	return false;
	    }

		sched_param param;
		param.sched_priority = 99;
		ret = pthread_attr_setschedparam(&attr, &param);
	    if(ret != 0)
	    {
	    	return false;
	    }
#endif
    }
    else if(priority < 0)
	{
#ifdef ANDROID_OS
		ret = pthread_attr_setschedpolicy(&attr, 3/*SCHED_BATCH*/);
	    if(ret != 0)
	    {
	    	return false;
	    }
		
		sched_param param;
		param.sched_priority = 0;
		ret = pthread_attr_setschedparam(&attr, &param);
	    if(ret != 0)
	    {
	    	return false;
	    }
#endif
	}
    
	ret = pthread_create(
        &m_thread, 
        &attr, 
        MH_Thread::stubMain, 
        this
    );
    if(ret != 0)
    {
    }

	pthread_attr_destroy(&attr);
#endif

	return true;
}

void MH_Thread::Stop()
{
#ifdef WIN32
	if(m_thread == NULL)
#else
	if(m_thread == 0)
#endif
	{
		return;
	}
	
	m_is_terminate = true;
	m_terminate.Set();
	
	OnReqTerminate();
	
#ifdef WIN32
	WaitForSingleObject(m_thread, INFINITE);
	CloseHandle(m_thread);
	m_thread = NULL;
#else
	void* thread_return = NULL;
	pthread_join(m_thread, &thread_return);
	m_thread = 0;
#endif

	OnStop();

	m_terminate.Close();
}

void MH_Thread::WaitForTerminate()
{
#ifdef WIN32
	if(m_thread == NULL)
#else
	if(m_thread == 0)
#endif
	{
		return;
	}
	
#ifdef WIN32
	WaitForSingleObject(m_thread, INFINITE);
	CloseHandle(m_thread);
	m_thread = NULL;
#else	
	void* thread_return = NULL;
	pthread_join(m_thread, &thread_return);
	m_thread = 0;
#endif
	
	m_is_terminate = true;

	OnStop();

	m_terminate.Close();
}

#ifdef WIN32
unsigned __stdcall MH_Thread::stubMain(void * arg)
#else
void* MH_Thread::stubMain(void * arg)
#endif
{
	MH_Thread* own = (MH_Thread*)arg;
	own->OnMain();

#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}

void MH_Thread::SetTerminate()
{
	m_is_terminate = true;
	m_terminate.Set();

	OnReqTerminate();
}

bool MH_Thread::IsTerminate()
{
	return m_is_terminate;
}

bool MH_Thread::ThreadSleep(int msec)
{
	return m_terminate.Wait(msec);
}
