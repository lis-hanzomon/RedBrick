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
#include "mhengine/MH_CriticalSection.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_CriticalSection::MH_CriticalSection()
{
#ifdef WIN32
	InitializeCriticalSectionEx(&m_sync, 2000, CRITICAL_SECTION_NO_DEBUG_INFO);
#else
	pthread_mutexattr_t mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &mta);
#endif
}

MH_CriticalSection::~MH_CriticalSection()
{
#ifdef WIN32
	DeleteCriticalSection(&m_sync);
#else
	pthread_mutex_destroy(&m_mutex);
#endif
}

void MH_CriticalSection::Lock()
{
#ifdef WIN32
	EnterCriticalSection(&m_sync);
#else
	pthread_mutex_lock(&m_mutex);
#endif
}

void MH_CriticalSection::Unlock()
{
#ifdef WIN32
	LeaveCriticalSection(&m_sync);
#else
	pthread_mutex_unlock(&m_mutex);
#endif
}

bool MH_CriticalSection::TryLock()
{
#ifdef WIN32
	if (TryEnterCriticalSection(&m_sync))
#else
	if (pthread_mutex_trylock(&m_mutex) == 0)
#endif
	{
		return true;
	}
	else
	{
		return false;
	}
}

MH_AutoLock::MH_AutoLock(MH_CriticalSection* lock)
	: m_lock(lock)
{
	m_lock->Lock();
}

MH_AutoLock::~MH_AutoLock()
{
	m_lock->Unlock();
}
