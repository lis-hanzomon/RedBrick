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
#include "mhengine/MH_UnixServerSocket.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h> 
#include <sys/un.h>
#include <sys/stat.h>

MH_UnixServerSocket::MH_UnixServerSocket(const char* path)
	: MH_ServerSocket()
{
	strcpy(m_path, path);
}

MH_UnixServerSocket::~MH_UnixServerSocket()
{
}

bool MH_UnixServerSocket::Create(int backlog)
{
	if (m_socket != INVALID_SOCKET)
	{
		return false;
	}

	unlink(m_path);

	m_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	int val = 1;
	ioctl(m_socket, FIONBIO, &val);

	unlink(m_path);

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, m_path);

	int ret = bind(m_socket, (struct sockaddr *)&addr, sizeof(addr));
	if(ret != 0)
	{
		Close();
		return false;
	}

	chmod(m_path, 0777);

	ret = listen(m_socket, backlog);
	if(ret != 0)
	{
		Close();
		return false;
	}
	
	m_cancel = false;

	return true;
}

MH_Socket* MH_UnixServerSocket::OnAccept()
{
	struct sockaddr_un cliaddr;
	memset(&cliaddr, 0, sizeof(struct sockaddr_un));
	socklen_t addrlen = sizeof(struct sockaddr_un);

	int client_socket = accept(m_socket, (struct sockaddr *)&cliaddr, &addrlen);
	
	return new MH_Socket(client_socket);
}
