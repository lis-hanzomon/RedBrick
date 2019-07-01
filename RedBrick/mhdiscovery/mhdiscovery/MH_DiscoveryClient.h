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

#ifndef MH_DISCOVERY_CLIENT_H
#define MH_DISCOVERY_CLIENT_H

#include "MH_Discovery.h"

#include "mhengine/MH_CriticalSection.h"
#include "mhengine/MH_Thread.h"
#include "mhengine/MH_UdpSocket.h"
#include "MH_DiscoveryThread.h"

#include <map>
#include <string>

class MHDISCOVERY_API MH_DiscoveryClient : public MH_Thread
										 , public MH_DiscoveryThread::EventHandler
{
public:
	class EventHandler
	{
	public:
		virtual ~EventHandler() {}
		
		virtual void OnDetectDevice(const char* endpoint_reference, const char* xaddr, const char* version) {}
		virtual void OnLostDevice(const char* endpoint_reference) {}
	};

private:
	EventHandler* m_handler;

	MH_DiscoveryThread* m_discoveryThread;
	
	char* m_type_ns;
	char* m_type;

	MH_UdpSocket* m_socket;
	
	MH_CriticalSection m_sync;
	MH_CriticalSection m_device_sync;

	std::map<std::string, std::string> m_device;

	bool CheckTypes(const char* types);

	void SendProbe();
	
	void DetectDevice(const char* endpoint_reference, const char* xaddr, const char* version);
	void LostDevice(const char* endpoint_reference, const char* version);

protected:
	virtual void OnMain();
	virtual void OnRecvMessage(sockaddr_in* addr, const char* message);

public:
	MH_DiscoveryClient(EventHandler* handler);
	virtual ~MH_DiscoveryClient();

	virtual bool Start(const char* net_interface, const char* type_ns, const char* type);
	virtual void Stop();
};

#endif
