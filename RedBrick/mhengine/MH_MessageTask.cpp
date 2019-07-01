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
#include "mhengine/MH_MessageTask.h"
#include "mhengine/MH_MessageQueueManager.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MessageTask::MH_MessageTask(int queue_id)
	: m_queue_id(queue_id)
	, m_message_queue(NULL)
{
}

MH_MessageTask::~MH_MessageTask()
{
}

bool MH_MessageTask::OnStart()
{
	if (!m_sync_event.Create())
	{
		return false;
	}
	
	MH_MessageQueueManager* queue_manager = MH_MessageQueueManager::GetInstance();
	
	if (m_queue_id == -1)
	{
		m_message_queue = queue_manager->CreateMessageQueue();
	}
	else
	{
		m_message_queue = queue_manager->GetMessageQueue(m_queue_id);
	}
	
	if (m_message_queue == NULL)
	{
		DBG_PRINT("Error: m_message_queue = NULL");
		m_sync_event.Close();
		return false;
	}

	return true;
}

void MH_MessageTask::OnReqTerminate()
{
	if (m_message_queue != NULL)
	{
		m_message_queue->Terminate();
	}
}

void MH_MessageTask::OnStop()
{
	if (m_queue_id == -1)
	{
		MH_MessageQueueManager* queue_manager = MH_MessageQueueManager::GetInstance();
		queue_manager->DestroyMessageQueue(m_message_queue->GetQueueId());
	}
	
	m_message_queue = NULL;
	
	m_sync_event.Close();
}

MH_MessageQueue* MH_MessageTask::GetMessageQueue()
{
	return m_message_queue;
}

int MH_MessageTask::GetQueueId()
{
	if (m_message_queue != NULL)
	{
		return m_message_queue->GetQueueId();
	}
	
	return m_queue_id;
}

void MH_MessageTask::OnMain()
{
	if (OnTaskInitialize())
	{
		while(!IsTerminate())
		{
			MH_Message* message = m_message_queue->RecvMessage();
			if(message == NULL)
			{
				break;
			}
			
			bool ret = OnMessage(message->message_id, message->data, message->size);
			
			m_message_queue->ReturnMessage(message, ret);
		}
	}
	
	OnTaskTerminate();
}

bool MH_MessageTask::OnTaskInitialize()
{
	return true;
}

void MH_MessageTask::OnTaskTerminate()
{
}
