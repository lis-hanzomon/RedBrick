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
#include "mhengine/MH_SocketTask.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_SocketTask::MH_SocketTask(EventHandler* handler, int send_buffer_size)
	: m_handler(handler)
	, m_socket(NULL)
	, m_status(false)
{
	if (send_buffer_size > 0)
	{
		m_send_buffer = (unsigned char*)malloc(send_buffer_size);
	}
	else
	{
		m_send_buffer = NULL;
	}
	m_send_buffer_size  = send_buffer_size;
	m_send_buffer_pos   = 0;
	m_send_buffer_store = 0;
}

MH_SocketTask::MH_SocketTask(EventHandler* handler, MH_Socket* socket, int send_buffer_size)
	: m_handler(handler)
	, m_socket(socket)
	, m_status(true)
{
	if (send_buffer_size > 0)
	{
		m_send_buffer = (unsigned char*)malloc(send_buffer_size);
	}
	else
	{
		m_send_buffer = NULL;
	}
	m_send_buffer_size  = send_buffer_size;
	m_send_buffer_pos   = 0;
	m_send_buffer_store = 0;
}

MH_SocketTask::~MH_SocketTask()
{
	if (m_send_buffer != NULL)
	{
		free(m_send_buffer);
		m_send_buffer = NULL;
	}
}

void MH_SocketTask::OnMain()
{
	while (!IsTerminate())
	{
		int network_event = m_socket->WaitEvent();
		if(network_event == -1)
		{
			break;
		}

		if(network_event & FD_WRITE)
		{
			OnSend();
		}
		
		if(network_event & FD_READ)
		{
			OnRecv();
		}
	}
}

void MH_SocketTask::OnRecv()
{
	m_handler->OnRecv(this);
}

void MH_SocketTask::OnSend()
{
	MH_AutoLock lock(&m_sync);

	while(m_send_buffer_store > 0)
	{
		int ret = m_socket->Send(m_send_buffer + m_send_buffer_pos, m_send_buffer_store);
		
		if(0 < ret)
		{
			m_send_buffer_store -= ret;
			m_send_buffer_pos   += ret;
		}
		else if(ret == 0)
		{
			break;
		}
		else
		{
			m_send_buffer_store = 0;
			m_send_buffer_pos   = 0;
			m_status = false;
			break;
		}
	}
}

bool MH_SocketTask::Connect(MH_Socket* socket)
{
	m_socket = socket;

	if (!m_socket->Connect())
	{
		delete m_socket;
		m_socket = NULL;
		return false;
	}

	Start();

	return true;
}

void MH_SocketTask::Close()
{
	if (m_socket != NULL)
	{
		m_socket->Cancel();
	}

	Stop();

	if (m_socket != NULL)
	{
		m_socket->Close();
	}
}

int MH_SocketTask::Send(unsigned char* buffer, int size)
{
	MH_AutoLock lock(&m_sync);
	
	if(!m_status)
	{
		return -1;
	}
	if(m_send_buffer_store != 0)
	{
		return 0;
	}

	int ret = m_socket->Send(buffer, size);
	if(ret == size)
	{
		return ret;
	}
	else if(ret < 0)
	{
		return -1;
	}
	else
	{
		if (m_send_buffer != NULL)
		{
			m_send_buffer_store = size - ret;
			if (m_send_buffer_store > m_send_buffer_size)
			{
				m_send_buffer_store = m_send_buffer_size;
			}
			memcpy(m_send_buffer, buffer + ret, m_send_buffer_store);
			m_send_buffer_pos = 0;
			return size;
		}
	}

	return ret;
}

int MH_SocketTask::Recv(unsigned char* buffer, int size)
{
	return m_socket->Recv(buffer, size);
}

void MH_SocketTask::Shutdown(bool recv_off, bool send_off)
{
	m_socket->Shutdown(recv_off, send_off);
}
