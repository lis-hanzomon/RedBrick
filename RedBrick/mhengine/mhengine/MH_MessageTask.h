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

#ifndef MH_MESSAGE_TASK_H
#define MH_MESSAGE_TASK_H

#include "MH_Engine.h"

#include "MH_Thread.h"
#include "MH_MessageQueue.h"

class MHENGINE_API MH_MessageTask : public MH_Thread
{
private:
	int m_queue_id;

	MH_MessageQueue* m_message_queue;

protected:
	MH_Event m_sync_event;

	virtual bool OnStart();
	virtual void OnMain();
	virtual void OnReqTerminate();
	virtual void OnStop();

	virtual bool OnTaskInitialize();
	virtual void OnTaskTerminate();

	virtual bool OnMessage(int message_id, void* data, int size) = 0;

public:
	MH_MessageTask(int queue_id = -1);
	virtual ~MH_MessageTask();
	
	MH_MessageQueue* GetMessageQueue();
	
	int GetQueueId();
};

#endif
