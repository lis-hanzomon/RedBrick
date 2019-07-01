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

#ifndef MH_DISCOVERY_THREAD_H
#define MH_DISCOVERY_THREAD_H

#include "MH_Discovery.h"

#include "mhengine/MH_Engine.h"
#include "mhengine/MH_Thread.h"
#include "mhengine/MH_UdpSocket.h"

#define WS_DISCOVERY_ADDRESS "239.255.255.250"
#define WS_DISCOVERY_PORT_NO 3702

class MHDISCOVERY_API MH_DiscoveryThread : public MH_Thread
{
public:
	class EventHandler
	{
	public:
		virtual ~EventHandler() {}
		virtual void OnRecvMessage(sockaddr_in* addr, const char* message) {}
	};

private:
	EventHandler* m_handler;

	MH_UdpSocket* m_socket;

protected:
	virtual void OnMain();

public:
	MH_DiscoveryThread(EventHandler* handler);
	virtual ~MH_DiscoveryThread();

	virtual bool Start(const char* net_interface);
	virtual void Stop();
};

#endif
