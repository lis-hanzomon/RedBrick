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
#include "mhengine/MH_ServerSocket.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#include <unistd.h>

#define INVALID_SOCKET -1

MH_ServerSocket::MH_ServerSocket()
	: m_socket(INVALID_SOCKET)
	, m_cancel(false)
{
}

MH_ServerSocket::~MH_ServerSocket()
{
}

bool MH_ServerSocket::Create(int backlog)
{
	return true;
}

void MH_ServerSocket::Close()
{
	if (m_socket != INVALID_SOCKET)
	{
		close(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

MH_Socket* MH_ServerSocket::Accept()
{
 	struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 100 * 1000;
	
	while(!m_cancel)
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(m_socket, &read_fds);
		
		int n = select(m_socket + 1, &read_fds, NULL, NULL, &tv);
		if(n > 0)
		{
			if(FD_ISSET(m_socket, &read_fds))
			{
				return OnAccept();
			}
		}
		else if(n < 0)
		{
			break;
		}
	}

	return NULL;
}

MH_Socket* MH_ServerSocket::OnAccept()
{
	return NULL;
}

void MH_ServerSocket::Cancel()
{
	m_cancel = true;
}
