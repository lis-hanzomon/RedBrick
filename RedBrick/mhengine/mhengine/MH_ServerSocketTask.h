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

#ifndef MH_SERVER_SOCKET_TASK_H
#define MH_SERVER_SOCKET_TASK_H

#include "MH_Engine.h"

#include "MH_Thread.h"
#include "MH_ServerSocket.h"

class MHENGINE_API MH_ServerSocketTask : public MH_Thread
{
public:
	class EventHandler
	{
	public:
		virtual ~EventHandler()
		{
		}
		
		virtual void OnConnect(MH_ServerSocketTask* own, MH_Socket* socket) = 0;
	};

private:
	EventHandler* m_handler;

	MH_ServerSocket* m_server_socket;

protected:
	virtual void OnMain();

public:
	MH_ServerSocketTask(EventHandler* handler);
	virtual ~MH_ServerSocketTask();

	virtual bool Open(MH_ServerSocket* server_socket, int backlog = 5);
	virtual void Close();
};

#endif
