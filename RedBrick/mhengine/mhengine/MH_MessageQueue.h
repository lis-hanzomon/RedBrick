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

#ifndef MH_MESSAGE_QUEUE_H
#define MH_MESSAGE_QUEUE_H

#include "MH_Engine.h"

#include <queue>

#include "MH_Event.h"
#include "MH_CriticalSection.h"

struct MH_Message
{
	int message_id;
	
	void* data;
	int size;
	
	MH_Event* sync_event;
	bool ret;
};

class MHENGINE_API MH_MessageQueue
{
private:
	int m_queue_id;

	std::queue<MH_Message*> m_queue;

	MH_Event			m_event;
	MH_CriticalSection	m_sync;

    bool m_created;
    bool m_terminate;

public:
	MH_MessageQueue();
	~MH_MessageQueue();

	bool Create(int queue_id);
	void Close();

	int GetQueueId();

	bool SendMessage(int message_id, void* data, int size, MH_Event* sync_event = NULL);

	MH_Message* RecvMessage();

	void ReturnMessage(MH_Message* message, bool ret);
	
	void Terminate();
};

#endif
