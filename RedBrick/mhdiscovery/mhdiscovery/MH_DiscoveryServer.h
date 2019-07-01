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

#ifndef MH_DISCOVERY_SERVER_H
#define MH_DISCOVERY_SERVER_H

#include "MH_Discovery.h"

#include "mhengine/MH_Engine.h"
#include "mhengine/MH_UdpSocket.h"
#include "MH_DiscoveryThread.h"

class MHDISCOVERY_API MH_DiscoveryServer : public MH_DiscoveryThread::EventHandler
{
private:
	MH_DiscoveryThread* m_discovery_thread;
	
	char* m_endpoint_reference;
	char* m_type_ns;
	char* m_type;
	char* m_scope;
	char* m_xaddr;
	char* m_version;
	
	int m_instance_id;
	int m_message_number;

	MH_UdpSocket* m_socket;
	
	struct sockaddr_in m_addr;

	void SendHello();
	void SendBye();
	void SendProbeMatch(sockaddr_in* addr, const char* relate_message_id);

protected:
	virtual void OnRecvMessage(sockaddr_in* addr, const char* message);

public:
	MH_DiscoveryServer();
	virtual ~MH_DiscoveryServer();

	virtual bool Start(const char* net_interface, const char* endpoint_reference, const char* type_ns, const char* type, const char* scope, const char* xaddr, const char* version);
	virtual void Stop();
};

#endif
