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

#ifndef MH_UDP_SOCKET_H
#define MH_UDP_SOCKET_H

#include "MH_Engine.h"

#ifndef WIN32
#include <netinet/in.h>
typedef int SOCKET;
#endif

class MHENGINE_API MH_UdpSocket
{
private:
	SOCKET m_socket;

	bool m_use_ipv6;

	bool m_is_membersip;
	ip_mreq m_mreq;
	ipv6_mreq m_mreq_v6;

#ifdef WIN32
	WSAEVENT m_socket_event;
	WSAEVENT m_cancel_event;
#endif
	bool m_cancel;

public:
	MH_UdpSocket(bool use_ipv6);
	virtual ~MH_UdpSocket();

	bool Create();
	void Close();
	
	bool Bind(short port);
	
	bool AddMemberShip(const char* multicast_address, const char* interface_address);
	void DropMemberShip();
	
	bool SetMulticastInterface(const char* interface_address);
	bool SetMulticastLoop(char loopch);
	
	int SendTo(void* buf, int len, sockaddr_in* dst_addr, socklen_t addr_len);
	int RecvFrom(void* buf, int len, sockaddr_in* src_addr, socklen_t* addr_len);

	bool WaitRecvEvent();
	void Cancel();
};

#endif
