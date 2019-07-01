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
 
  #ifndef KII_SOCKET_TASK_H
#define KII_SOCKET_TASK_H

#include "MH_Engine.h"

#include "MH_Thread.h"
#include "MH_Socket.h"
#include "MH_CriticalSection.h"

class MHENGINE_API MH_SocketTask : public MH_Thread
{
public:
	class EventHandler
	{
	public:
		virtual ~EventHandler()
		{
		}
	
		virtual void OnRecv(MH_SocketTask* own) = 0;
	};

private:
	EventHandler* m_handler;
	MH_Socket* m_socket;

	MH_CriticalSection m_sync;

	bool m_status;
	
	unsigned char* m_send_buffer;
	int m_send_buffer_size;
	int m_send_buffer_pos;
	int m_send_buffer_store;

protected:
	virtual void OnMain();

	virtual void OnRecv();
	virtual void OnSend();

public:
	MH_SocketTask(EventHandler* handler, int send_buffer_size = 0);
	MH_SocketTask(EventHandler* handler, MH_Socket* socket, int send_buffer_size = 0);
	virtual ~MH_SocketTask();

	bool Connect(MH_Socket* socket);
	void Close();

	int Send(unsigned char* buffer, int size);
	int Recv(unsigned char* buffer, int size);

	void Shutdown(bool recv_off, bool send_off);
};

#endif
