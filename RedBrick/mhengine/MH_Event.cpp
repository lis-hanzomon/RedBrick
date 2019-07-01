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
#include "mhengine/MH_Event.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#ifndef WIN32
#include <errno.h>
#endif

MH_Event::MH_Event()
#ifdef WIN32
	: m_event(NULL)
#else
	: m_created(false)
#endif
{
}

MH_Event::~MH_Event()
{
}

bool MH_Event::Create(bool manual, bool initial_state)
{
#ifdef WIN32
	if(m_event != NULL)
	{
		return true;
	}
	
	m_event = CreateEvent(NULL, manual, FALSE, NULL);
#else
	if(m_created)
	{
		return true;
	}
	
	pthread_mutex_init(&m_mutex, NULL);
	
	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&m_condition, &attr);
	
	m_manual  = manual;
	m_state   = initial_state;
	m_created = true;
#endif
	
	return true;
}

#ifdef WIN32

bool MH_Event::CreateEx(const wchar_t* name, bool manual, bool initial_state)
{
	if (m_event != NULL)
	{
		return true;
	}

	SECURITY_DESCRIPTOR desc;
	InitializeSecurityDescriptor(&desc, SECURITY_DESCRIPTOR_REVISION);
	if (!SetSecurityDescriptorDacl(&desc, TRUE, NULL, FALSE))
	{
		return false;
	}

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = &desc;

	m_event = CreateEvent(&sa, manual, FALSE, name);
	if (m_event == NULL)
	{
		return false;
	}

	return true;
}

bool MH_Event::OpenEx(const wchar_t* name)
{
	if (m_event != NULL)
	{
		return true;
	}

	m_event = OpenEvent(SYNCHRONIZE, FALSE, name);
	if (m_event == NULL)
	{
		DWORD error = GetLastError();
		return false;
	}

	return true;
}

#endif

void MH_Event::Close()
{
#ifdef WIN32
	if (!m_event)
	{
		return;
	}
	
	CloseHandle(m_event);
	m_event = NULL;
#else
	if (!m_created)
	{
		return;
	}
	
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_condition);
	m_created = false;
#endif
}

bool MH_Event::Set()
{
#ifdef WIN32
	if (!m_event)
	{
		return false;
	}

	SetEvent(m_event);
#else	
	if (!m_created)
	{
		return false;
	}
	
	pthread_mutex_lock(&m_mutex);

    m_state = true;
    pthread_cond_signal(&m_condition);

    pthread_mutex_unlock(&m_mutex);		
#endif
	
	return true;
}

bool MH_Event::Reset()
{
#ifdef WIN32
	if (!m_event)
	{
		return false;
	}

	ResetEvent(m_event);
#else
	if (!m_created)
	{
		return false;
	}

	pthread_mutex_lock(&m_mutex);

    m_state = false;

    pthread_mutex_unlock(&m_mutex);
#endif
	
	return true;
}

bool MH_Event::Wait(long timeout)
{
#ifdef WIN32
	if (!m_event)
	{
		return false;
	}
	
	if(WaitForSingleObject(m_event, timeout) == WAIT_OBJECT_0)
	{
		return true;
	}
	else
	{
		return false;
	}
#else
	if (!m_created)
	{
		return false;
	}
	
	pthread_mutex_lock(&m_mutex);

    while (!m_state)
    {
    	if(timeout >= 0)
    	{
#ifdef ANDROID_OS
			if (pthread_cond_timeout_np(&m_condition, &m_mutex, timeout) == ETIMEDOUT)
#else
			int delta_sec   = timeout / 1000;
			long delta_nsec = (timeout % 1000) * 1000 * 1000;
			
			struct timespec now;
			clock_gettime(CLOCK_MONOTONIC, &now);
			
			struct timespec ts = now;
			ts.tv_nsec += delta_nsec;
			
			if (ts.tv_nsec < 1000 * 1000 * 1000)
			{
				ts.tv_sec  += delta_sec;
			}
			else
			{
				ts.tv_sec  += (delta_sec + 1);
				ts.tv_nsec -= 1000 * 1000 * 1000;
			}

			int err = pthread_cond_timedwait(&m_condition, &m_mutex, &ts);
			if (err == ETIMEDOUT)
#endif
	        {
	        	break;
	        }
	        else if (err != 0)
	        {
	        	DBG_PRINT("Err: pthread_cond_timedwait = %d", err);
	        }
	    }
	    else
	    {
	    	pthread_cond_wait(&m_condition, &m_mutex);
	    }
    }

	bool ret = m_state;

	if(!m_manual)
	{
	    m_state = false;
	}

    pthread_mutex_unlock(&m_mutex);
    
    return ret;
#endif
}
