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

#ifndef MH_EVENT_H
#define MH_EVENT_H

#include "MH_Engine.h"

#ifndef WIN32
#include <pthread.h>
#endif

class MHENGINE_API MH_Event
{
private:
#ifdef WIN32
	HANDLE m_event;
#else
	pthread_mutex_t m_mutex;
    pthread_cond_t  m_condition;
    bool            m_created;
    bool			m_manual;
    bool			m_state;
#endif

public:
	MH_Event();
	virtual ~MH_Event();

	bool Create(bool bManual = false, bool bInitialState = false);

#ifdef WIN32
	bool CreateEx(const wchar_t* name, bool bManual = false, bool bInitialState = false);
	bool OpenEx(const wchar_t* name);
#endif

	void Close();

	bool Set();
	bool Reset();
	bool Wait(long timeout = -1);
};

#endif
