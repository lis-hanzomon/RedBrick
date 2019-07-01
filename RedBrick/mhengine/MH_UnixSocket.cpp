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
#include "mhengine/MH_UnixSocket.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h> 
#include <sys/un.h>

#define INVALID_SOCKET	-1

MH_UnixSocket::MH_UnixSocket(const char* path)
	: MH_Socket()
{
	strcpy(m_path, path);
}

MH_UnixSocket::~MH_UnixSocket()
{
}

bool MH_UnixSocket::Connect()
{
	m_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	m_cancel = false;

	int val = 1;
	ioctl(m_socket, FIONBIO, &val);

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, m_path);

    int ret = connect(m_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (ret < 0)
    {
		Close();
		return false;
    }
	
	return true;
}
