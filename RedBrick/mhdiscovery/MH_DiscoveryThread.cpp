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
#include "mhdiscovery/MH_DiscoveryThread.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_DiscoveryThread::MH_DiscoveryThread(EventHandler* handler)
	: m_handler(handler)
	, m_socket(NULL)
{
}

MH_DiscoveryThread::~MH_DiscoveryThread()
{
}

bool MH_DiscoveryThread::Start(const char* net_interface)
{
	m_socket = new MH_UdpSocket(false);
	
	if (!m_socket->Create())
	{
		Stop();
		return false;
	}
	
	if (!m_socket->SetMulticastLoop(0))
	{
		Stop();
		return false;
	}
	
	if (!m_socket->Bind(WS_DISCOVERY_PORT_NO))
	{
		Stop();
		return false;
	}
	
	if (!m_socket->AddMemberShip(WS_DISCOVERY_ADDRESS, net_interface))
	{
		Stop();
		return false;
	}

	if (!MH_Thread::Start())
	{
		Stop();
		return false;
	}
	
	return true;
}

void MH_DiscoveryThread::Stop()
{
	if (m_socket != NULL)
	{
		m_socket->Cancel();
	}
	
	MH_Thread::Stop();

	if (m_socket != NULL)
	{
		m_socket->DropMemberShip();
		m_socket->Close();
		delete m_socket;
		m_socket = NULL;
	}
}

void MH_DiscoveryThread::OnMain()
{
	while (!IsTerminate())
	{
		if (m_socket->WaitRecvEvent())
		{
			char buf[2048];
			struct sockaddr_in from;
			socklen_t addr_len = sizeof(from);
			int len = m_socket->RecvFrom(buf, sizeof(buf) - 1, &from, &addr_len);
			if (len > 0)
			{
				buf[len] = 0x00;
				
				if (m_handler != NULL)
				{
					m_handler->OnRecvMessage(&from, buf);
				}
			}
		}
	}
}
