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

#ifndef MH_CRITICAL_SECTION_H
#define MH_CRITICAL_SECTION_H

#include "MH_Engine.h"

#ifndef WIN32
#include <pthread.h>
#endif

class MHENGINE_API MH_CriticalSection
{
private:
#ifdef WIN32
	CRITICAL_SECTION m_sync;
#else
	pthread_mutex_t m_mutex;
#endif
	
public:
	MH_CriticalSection();
	~MH_CriticalSection();

	void Lock();
	void Unlock();
	bool TryLock();
};

class MHENGINE_API MH_AutoLock
{
private:
	MH_CriticalSection* m_lock;

public:
	MH_AutoLock(MH_CriticalSection* lock);
	~MH_AutoLock();
};

#endif
