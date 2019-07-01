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

#ifndef MH_SERVER_SOCKET_H
#define MH_SERVER_SOCKET_H

#include "MH_Engine.h"

#include "MH_Socket.h"

class MHENGINE_API MH_ServerSocket
{
protected:
	int m_socket;
	bool m_cancel;
	
protected:
	virtual MH_Socket* OnAccept();

public:
	MH_ServerSocket();
	virtual ~MH_ServerSocket();

	virtual bool Create(int backlog = 5);
	virtual void Close();
	
	virtual MH_Socket* Accept();
	virtual void Cancel();
};

#endif
