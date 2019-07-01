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

#ifndef MH_MESSAGE_QUEUE_MANAGER_H
#define MH_MESSAGE_QUEUE_MANAGER_H

#include "MH_Engine.h"

#include "MH_MessageQueue.h"

class MHENGINE_API MH_MessageQueueManager
{
private:
	static MH_MessageQueueManager* m_instance;

	int m_queue_num;
	MH_MessageQueue** m_queue;

	MH_MessageQueueManager();
	~MH_MessageQueueManager();

	bool Create(int queue_num);
	void Destroy();
	
public:
	static bool Initialize(int queue_num);
	static void Terminate();
	
	static MH_MessageQueueManager* GetInstance();
	
	MH_MessageQueue* CreateMessageQueue(int queue_id = -1);
	void DestroyMessageQueue(int queue_id);
	
	MH_MessageQueue* GetMessageQueue(int queue_id);
};

#endif
