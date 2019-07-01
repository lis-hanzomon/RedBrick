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

#ifndef MH_UNIX_SOCKET_H
#define MH_UNIX_SOCKET_H

#include "MH_Engine.h"
#include "MH_Socket.h"

class MHENGINE_API MH_UnixSocket : public MH_Socket
{
private:
	char m_path[255];

public:
	MH_UnixSocket(const char* path);
	virtual ~MH_UnixSocket();

	virtual bool Connect();
};

#endif
