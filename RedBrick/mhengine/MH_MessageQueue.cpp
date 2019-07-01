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
#include "mhengine/MH_MessageQueue.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MessageQueue::MH_MessageQueue()
	: m_queue_id(-1)
	, m_created(false)
	, m_terminate(true)
{
}

MH_MessageQueue::~MH_MessageQueue()
{
	Close();
}

bool MH_MessageQueue::Create(int queue_id)
{
	if (m_created)
	{
		return true;
	}

	if (!m_event.Create())
	{
		return false;
	}

	m_sync.Lock();

	m_queue_id = queue_id;
	m_created   = true;
	m_terminate = false;
	
	m_sync.Unlock();
	
	return true;
}

void MH_MessageQueue::Close()
{
	if (!m_created)
	{
		return;
	}
	
	m_sync.Lock();

	m_queue_id  = -1;
	m_created   = false;
	m_terminate = true;
	
	while (!m_queue.empty())
	{
		MH_Message* message = m_queue.front();
		m_queue.pop();
		
		if (message->sync_event != NULL)
		{
			message->sync_event->Set();
		}
		else
		{
			if (message->data != NULL)
			{
				free(message->data);
			}
		}
		
		delete message;
	}

	m_sync.Unlock();

	m_event.Close();
}

int MH_MessageQueue::GetQueueId()
{
	return m_queue_id;
}

bool MH_MessageQueue::SendMessage(int message_id, void* data, int size, MH_Event* sync_event)
{
	m_sync.Lock();
	
	if (m_terminate)
	{
		m_sync.Unlock();
		return false;
	}
	
	MH_Message* message = new MH_Message();
	message->message_id = message_id;
	message->size = size;
	message->sync_event = sync_event;

	if(sync_event != NULL)
	{
		message->data = data;
	}
	else
	{
		message->data = new unsigned char[size];
		memcpy(message->data, data, size);
	}

	m_queue.push(message);
	
	m_sync.Unlock();

	m_event.Set();

	bool ret = true;
	
	if(sync_event != NULL)
	{
		sync_event->Wait();
		
		ret = message->ret;
		delete message;
	}
	
	return ret;
}

MH_Message* MH_MessageQueue::RecvMessage()
{
	m_sync.Lock();

	if (m_terminate)
	{
		m_sync.Unlock();
		return NULL;
	}

	while (m_queue.empty())
	{
		m_sync.Unlock();
		
		m_event.Wait();
		
		if (m_terminate)
		{
			return NULL;
		}
		
		m_sync.Lock();
	}

	MH_Message* message = m_queue.front();
	m_queue.pop();

	m_sync.Unlock();

	return message;
}

void MH_MessageQueue::ReturnMessage(MH_Message* message, bool ret)
{
	if (message->sync_event != NULL)
	{
		message->ret = ret;
		
		message->sync_event->Set();
	}
	else
	{
		if (message->data != NULL)
		{
			free(message->data);
		}
		
		delete message;
	}
}

void MH_MessageQueue::Terminate()
{
	m_terminate = true;
	m_event.Set();
}
