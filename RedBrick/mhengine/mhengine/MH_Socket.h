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

#ifndef MH_SOCKET_H
#define MH_SOCKET_H

#include "MH_Engine.h"

#define INVALID_SOCKET	-1

#define SD_BOTH		SHUT_RDWR
#define SD_RECEIVE	SHUT_RD
#define SD_SEND		SHUT_WR

#define FD_WRITE	0x01
#define FD_READ		0x02

class MHENGINE_API MH_Socket
{
protected:
	int m_socket;

	bool m_cancel;
	bool m_writable;

public:
	MH_Socket();
	MH_Socket(int socket);
	virtual ~MH_Socket();

	virtual bool Connect();
	virtual void Close();
	
	virtual int Send(void* buf, int len);
	virtual int Recv(void* buf, int len);

	virtual int WaitEvent();
	virtual void Cancel();
	
	void Shutdown(bool recv_off, bool send_off);
};

#endif
