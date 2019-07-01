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
#include "mhengine/MH_UdpSocket.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>

#define INVALID_SOCKET	-1
#endif

MH_UdpSocket::MH_UdpSocket(bool use_ipv6)
	: m_socket(INVALID_SOCKET)
	, m_use_ipv6(use_ipv6)
	, m_is_membersip(false)
#ifdef WIN32
	, m_socket_event(WSA_INVALID_EVENT)
	, m_cancel_event(WSA_INVALID_EVENT)
#endif
	, m_cancel(false)
{
}

MH_UdpSocket::~MH_UdpSocket()
{
}

bool MH_UdpSocket::Create()
{
	if (m_use_ipv6)
	{
		m_socket = socket(AF_INET6, SOCK_DGRAM, 0);
	}
	else
	{
		m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	}

#ifdef WIN32
	m_socket_event = WSACreateEvent();
	m_cancel_event = WSACreateEvent();
#endif
	m_cancel = false;

#ifdef WIN32
	u_long val = 1;
	ioctlsocket(m_socket, FIONBIO, &val);

	WSAEventSelect(m_socket, m_socket_event, FD_READ);
#else
	int val = 1;
	ioctl(m_socket, FIONBIO, &val);
#endif
	
	return true;
}

void MH_UdpSocket::Close()
{
	DropMemberShip();

	if (m_socket != INVALID_SOCKET)
	{
#ifdef WIN32
		closesocket(m_socket);
#else
		close(m_socket);
#endif
		m_socket = INVALID_SOCKET;
	}

#ifdef WIN32
	if(m_socket_event != WSA_INVALID_EVENT)
	{
		WSACloseEvent(m_socket_event);
		m_socket_event = WSA_INVALID_EVENT;
	}

	if(m_cancel_event != WSA_INVALID_EVENT)
	{
		WSACloseEvent(m_cancel_event);
		m_cancel_event = WSA_INVALID_EVENT;
	}
#endif
}

bool MH_UdpSocket::Bind(short port)
{
	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}
	
	if (m_use_ipv6)
	{
		struct sockaddr_in6 addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_port = htons(port);
		addr.sin6_addr = in6addr_any;
	
		if (bind(m_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0)
		{
			return false;
		}
	}
	else
	{
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
#ifdef WIN32
		addr.sin_addr.S_un.S_addr = INADDR_ANY;
#else
		addr.sin_addr.s_addr = INADDR_ANY;
#endif
	
		if (bind(m_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0)
		{
			return false;
		}
	}
	
	return true;
}

bool MH_UdpSocket::AddMemberShip(const char* multicast_address, const char* interface_address)
{
	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}
	
	if (m_use_ipv6)
	{
		memset(&m_mreq_v6, 0, sizeof(m_mreq_v6));
		
		if (interface_address == NULL)
		{
			m_mreq_v6.ipv6mr_interface = 0;
		}
		else
		{
			m_mreq_v6.ipv6mr_interface = if_nametoindex(interface_address);
		}
		
		inet_pton(AF_INET6, multicast_address, &(m_mreq_v6.ipv6mr_multiaddr));
		
    	if (setsockopt(m_socket, 
    		IPPROTO_IPV6, 
    		IPV6_JOIN_GROUP, 
    		&m_mreq_v6, sizeof(&m_mreq_v6)) != 0)
		{
			return false;
    	}
	}
	else
	{
		memset(&m_mreq, 0, sizeof(m_mreq));
		
		if (interface_address == NULL)
		{
#ifdef WIN32
			m_mreq.imr_interface.S_un.S_addr = INADDR_ANY;
#else
			m_mreq.imr_interface.s_addr = INADDR_ANY;
#endif
		}
		else
		{
#ifdef WIN32
			m_mreq.imr_interface.S_un.S_addr = inet_addr(interface_address);
#else
			m_mreq.imr_interface.s_addr = inet_addr(interface_address);
#endif
		}
		
#ifdef WIN32
		m_mreq.imr_multiaddr.S_un.S_addr = inet_addr(multicast_address);
#else
		m_mreq.imr_multiaddr.s_addr = inet_addr(multicast_address);
#endif

		if (setsockopt(m_socket,
			IPPROTO_IP,
			IP_ADD_MEMBERSHIP,
			(char *)&m_mreq, sizeof(m_mreq)) != 0)
		{
			return false;
		}
	}
	
	m_is_membersip = true;
	
	return true;
}

void MH_UdpSocket::DropMemberShip()
{
	if (m_socket == INVALID_SOCKET)
	{
		return;
	}
	if (!m_is_membersip)
	{
		return;
	}
	m_is_membersip = false;
	
	if (m_use_ipv6)
	{
		setsockopt(m_socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (char*)&m_mreq_v6, sizeof(m_mreq_v6));
	}
	else
	{
		setsockopt(m_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&m_mreq, sizeof(m_mreq));
	}
}

bool MH_UdpSocket::SetMulticastInterface(const char* interface_address)
{
	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}

	if (m_use_ipv6)
	{
 		int ifidx = if_nametoindex(interface_address);
		if (setsockopt(m_socket,
			IPPROTO_IPV6,
			IPV6_MULTICAST_IF,
			(char *)&ifidx, sizeof(ifidx)) != 0)
		{
			return false;
		}
	}
	else
	{
		unsigned long ipaddr = inet_addr(interface_address);
		if (setsockopt(m_socket,
			IPPROTO_IP,
			IP_MULTICAST_IF,
			(char *)&ipaddr, sizeof(ipaddr)) != 0)
		{
			return false;
		}
	}
	
	return true;
}

bool MH_UdpSocket::SetMulticastLoop(char loopch)
{
	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}
	
	if (m_use_ipv6)
	{
		int loop = loopch;
		if (setsockopt(m_socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
			(char *)&loop, sizeof(loop)) != 0)
		{
			return false;
		}
	}
	else
	{
		if (setsockopt(m_socket, IPPROTO_IP, IP_MULTICAST_LOOP,
			(char *)&loopch, sizeof(loopch)) != 0)
		{
			return false;
		}
	}
	
	return true;
}

int MH_UdpSocket::SendTo(void* buf, int len, sockaddr_in* dst_addr, socklen_t addr_len)
{
	if (m_socket == INVALID_SOCKET)
	{
		return -1;
	}
	
	return sendto(m_socket, buf, len, 0, (struct sockaddr *)dst_addr, addr_len);
}

int MH_UdpSocket::RecvFrom(void* buf, int len, sockaddr_in* src_addr, socklen_t* addr_len)
{
	if (m_socket == INVALID_SOCKET)
	{
		return -1;
	}
	
	return recvfrom(m_socket, buf, len, 0, (sockaddr*)src_addr, addr_len);
}

bool MH_UdpSocket::WaitRecvEvent()
{
	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}

#ifdef WIN32
	HANDLE wait_event[2] = { m_socket_event, m_cancel_event };

	while(!m_cancel)
	{
		int ret = WSAWaitForMultipleEvents(2, wait_event, FALSE, WSA_INFINITE, FALSE);
		if (ret != WSA_WAIT_EVENT_0)
		{
			break;
		}

		WSANETWORKEVENTS network_events;
		WSAEnumNetworkEvents(m_socket, m_socket_event, &network_events);
		if (network_events.lNetworkEvents & FD_READ)
		{
			return true;
		}
	}
#else
	int ret = 0;
	while(!m_cancel)
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(m_socket, &read_fds);
		
	 	struct timeval tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 100 * 1000;
		
		int n = select(m_socket + 1, &read_fds, NULL, NULL, &tv);
		if(n > 0)
		{
			if(FD_ISSET(m_socket, &read_fds))
			{
				return true;
			}
		}
		else if(n < 0)
		{
			break;
		}
	}
#endif

	return false;
}

void MH_UdpSocket::Cancel()
{
	m_cancel = true;
	
#ifdef WIN32
	WSASetEvent(m_cancel_event);
#endif
}
