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
#include "mhengine/MH_Socket.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h> 
#include <sys/un.h>

MH_Socket::MH_Socket()
	: m_socket(INVALID_SOCKET)
	, m_cancel(false)
	, m_writable(true)
{
}

MH_Socket::MH_Socket(int socket)
	: m_socket(socket)
	, m_cancel(false)
	, m_writable(true)
{
}

MH_Socket::~MH_Socket()
{
}

bool MH_Socket::Connect()
{
	return true;
}

void MH_Socket::Close()
{
	if (m_socket != INVALID_SOCKET)
	{
		close(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

int MH_Socket::Send(void* buf, int len)
{
	if (m_socket == INVALID_SOCKET)
	{
		return -1;
	}
	
	int ret = write(m_socket, buf, len);
	if (ret < 0)
	{
		return -1;
	}
	
	if (ret < len)
	{
		m_writable = false;
	}

	return ret;
}

int MH_Socket::Recv(void* buf, int len)
{
	if (m_socket == INVALID_SOCKET)
	{
		return -1;
	}
	
	return read(m_socket, buf, len);
}

int MH_Socket::WaitEvent()
{
	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}

	int ret = 0;
	while(!m_cancel)
	{
	 	struct timeval tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 100 * 1000;
		
		fd_set read_fds, write_fds;
		
		FD_ZERO(&read_fds);
		FD_SET(m_socket, &read_fds);
		
		int n = 0;
		
		if (!m_writable)
		{
			FD_ZERO(&write_fds);
			FD_SET(m_socket, &write_fds);

			n = select(m_socket + 1, &read_fds, &write_fds, NULL, &tv);
		}
		else
		{
			n = select(m_socket + 1, &read_fds, NULL, NULL, &tv);
		}
		
		if(n > 0)
		{
			int ret = 0;
			
			if(FD_ISSET(m_socket, &read_fds))
			{
				ret |= FD_READ;
			}
			if (!m_writable)
			{
				if(FD_ISSET(m_socket, &write_fds))
				{
					ret |= FD_WRITE;
				}
				m_writable = true;
			}
			
			if (ret != 0)
			{
				return ret;
			}
		}
		else if(n < 0)
		{
			break;
		}
	}

	return false;
}

void MH_Socket::Cancel()
{
	m_cancel = true;
}

void MH_Socket::Shutdown(bool recv_off, bool send_off)
{
	int how = 0;

	if(recv_off && send_off)
	{
		how = SD_BOTH;
	}
	else if(recv_off)
	{
		how = SD_RECEIVE;
	}
	else if(send_off)
	{
		how = SD_SEND;
	}
	else
	{
		return;
	}

	shutdown(m_socket, how);
}
