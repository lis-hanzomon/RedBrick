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
#include "mhengine/MH_MessageQueueManager.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MessageQueueManager* MH_MessageQueueManager::m_instance = NULL;

bool MH_MessageQueueManager::Initialize(int queue_num)
{
	if (m_instance != NULL)
	{
		return false;
	}
	
	m_instance = new MH_MessageQueueManager();
	
	bool ret = m_instance->Create(queue_num);
	if (!ret)
	{
		Terminate();
	}

	return ret;
}

void MH_MessageQueueManager::Terminate()
{
	if (m_instance == NULL)
	{
		return;
	}
	
	m_instance->Destroy();

	delete m_instance;
	m_instance = NULL;
}

MH_MessageQueueManager* MH_MessageQueueManager::GetInstance()
{
	return m_instance;
}

MH_MessageQueueManager::MH_MessageQueueManager()
	: m_queue_num(0)
	, m_queue(NULL)
{
}

MH_MessageQueueManager::~MH_MessageQueueManager()
{
}

bool MH_MessageQueueManager::Create(int queue_num)
{
	m_queue_num = queue_num;

	m_queue = new MH_MessageQueue* [queue_num];
	
	for (int i = 0; i < queue_num; i++)
	{
		m_queue[i] = NULL;
	}

	return true;
}

void MH_MessageQueueManager::Destroy()
{
	if (m_queue != NULL)
	{
		for (int i = 0; i < m_queue_num; i++)
		{
			if (m_queue[i] != NULL)
			{
				m_queue[i]->Close();
				delete m_queue[i];
			}
		}
		
		delete [] m_queue;
		m_queue = NULL;
	}
}
	
MH_MessageQueue* MH_MessageQueueManager::CreateMessageQueue(int queue_id)
{
	if (queue_id == -1)
	{
		for (int i = 0; i < m_queue_num; i++)
		{
			if (m_queue[i] == NULL)
			{
				queue_id = i;
				break;
			}
		}
		if(queue_id == -1)
		{
			return NULL;
		}
	}
	else
	{
		if (m_queue[queue_id] != NULL)
		{
			return NULL;
		}
	}
	
	MH_MessageQueue* message_queue = new MH_MessageQueue();
	if (!message_queue->Create(queue_id))
	{
		delete message_queue;
		return NULL;
	}
	
	m_queue[queue_id] = message_queue;
	
	return message_queue;
}

void MH_MessageQueueManager::DestroyMessageQueue(int queue_id)
{
	if (m_queue[queue_id] == NULL)
	{
		return;
	}
	
	m_queue[queue_id]->Close();
	delete m_queue[queue_id];
	m_queue[queue_id] = NULL;
}

MH_MessageQueue* MH_MessageQueueManager::GetMessageQueue(int queue_id)
{
	return m_queue[queue_id];
}
