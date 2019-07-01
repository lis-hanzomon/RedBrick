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
#include "mhdiscovery/MH_DiscoveryClient.h"
#include "MH_DiscoveryParser.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <iphlpapi.h>
#else
#include <arpa/inet.h>
#endif

MH_DiscoveryClient::MH_DiscoveryClient(EventHandler* handler)
	: m_handler(handler)
	, m_discoveryThread(NULL)
	, m_type_ns(NULL)
	, m_type(NULL)
	, m_socket(NULL)
{
}

MH_DiscoveryClient::~MH_DiscoveryClient()
{
}

bool MH_DiscoveryClient::Start(const char* net_interface, const char* type_ns, const char* type)
{
	if (net_interface == NULL || type_ns == NULL || type == NULL)
	{
		return false;
	}
	if (m_socket != NULL)
	{
		return false;
	}

	m_type_ns = strdup(type_ns);
	m_type    = strdup(type);

	m_socket = new MH_UdpSocket(false);
	if (!m_socket->Create())
	{
		Stop();
		return false;
	}

	if (!m_socket->SetMulticastInterface(net_interface))
	{
		Stop();
		return false;
	}

	if (!m_socket->SetMulticastLoop(0))
	{
		Stop();
		return false;
	}

	m_discoveryThread = new MH_DiscoveryThread(this);
	if (!m_discoveryThread->Start(net_interface))
	{
		Stop();
		return false;
	}
	
	if (!MH_Thread::Start())
	{
		Stop();
		return false;
	}
	
	SendProbe();
	
	return true;
}

void MH_DiscoveryClient::Stop()
{
	if (m_discoveryThread != NULL)
	{
		m_discoveryThread->Stop();
		delete m_discoveryThread;
		m_discoveryThread = NULL;
	}
	
	if (m_socket != NULL)
	{
		m_socket->Cancel();
	}
	
	MH_Thread::Stop();
	
	if (m_socket != NULL)
	{
		m_socket->Close();
		delete m_socket;
		m_socket = NULL;
	}
	
	if (m_type_ns != NULL)
	{
		free(m_type_ns);
		m_type_ns = NULL;
	}
	if (m_type != NULL)
	{
		free(m_type);
		m_type = NULL;
	}
}

void MH_DiscoveryClient::OnMain()
{
	while(!IsTerminate())
	{
		if (m_socket->WaitRecvEvent())
		{
			char buf[2048];
			sockaddr_in from;
			socklen_t addr_len = sizeof(from);
			int len = m_socket->RecvFrom(buf, sizeof(buf) - 1, &from, &addr_len);
			if (len > 0)
			{
				buf[len] = 0x00;

				MSG_DATA msg_data;
				memset(&msg_data, 0x00, sizeof(msg_data));

				if (MH_DiscoveryParser::Parser(buf, len, &msg_data))
				{
					if (msg_data.action_type == ACTION_TYPE_PROBE_MATCHES)
					{
						if (MH_DiscoveryParser::CheckTypes(msg_data.body_types, m_type_ns, m_type))
						{
							DetectDevice(
								msg_data.body_endpoint_reference,
								msg_data.body_x_addrs,
								msg_data.body_metadata_version
								);
						}
					}
				}
			}
		}
	}
}

void MH_DiscoveryClient::OnRecvMessage(sockaddr_in* addr, const char* message)
{
	MSG_DATA msg_data;
	memset(&msg_data, 0x00, sizeof(msg_data));

	if (MH_DiscoveryParser::Parser(message, strlen(message), &msg_data))
	{
		if (msg_data.action_type == ACTION_TYPE_HELLO)
		{
			if (MH_DiscoveryParser::CheckTypes(msg_data.body_types, m_type_ns, m_type))
			{
				SendProbe();
			}
		}
		else if (msg_data.action_type == ACTION_TYPE_BYE)
		{
			LostDevice(msg_data.body_endpoint_reference, msg_data.body_metadata_version);
		}
	}
}

void MH_DiscoveryClient::SendProbe()
{
	MH_AutoLock lock(&m_sync);

	char uuid[80];
	UuidGenerate(uuid);

	char cmd[1024 * 10];
	sprintf(cmd,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<s:Envelope xmlns:dp0=\"http://www.onvif.org/ver10/network/wsdl\" xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">"
			"<s:Header>"
				"<a:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</a:Action>"
				"<a:MessageID>uuid:%s</a:MessageID>"
			"</s:Header>"
			"<s:Body>"
				"<d:Probe>"
					"<d:Types>dp0:NetworkVideoTransmitter</d:Types>"
				"</d:Probe>"
			"</s:Body>"
		"</s:Envelope>",
		uuid
	);

	struct sockaddr_in addr;
	memset(&addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(WS_DISCOVERY_PORT_NO);
#ifdef WIN32
	addr.sin_addr.S_un.S_addr = inet_addr(WS_DISCOVERY_ADDRESS);
#else
	addr.sin_addr.s_addr = inet_addr(WS_DISCOVERY_ADDRESS);
#endif

	m_socket->SendTo(cmd, strlen(cmd), &addr, sizeof(addr));
}

void MH_DiscoveryClient::DetectDevice(const char* endpoint_reference, const char* xaddr, const char* version)
{
	MH_AutoLock lock(&m_device_sync);

	std::map<std::string, std::string>::iterator it = m_device.find(endpoint_reference);
	if (it != m_device.end())
	{
		if (strcmp((*it).second.c_str(), version) == 0)
		{
			return;
		}
	}
	
	m_device[endpoint_reference] = version;
	
	if (m_handler != NULL)
	{
		m_handler->OnDetectDevice(endpoint_reference, xaddr, version);
	}
}

void MH_DiscoveryClient::LostDevice(const char* endpoint_reference, const char* version)
{
	MH_AutoLock lock(&m_device_sync);

	std::map<std::string, std::string>::iterator it = m_device.find(endpoint_reference);
	if (it == m_device.end())
	{
		return;
	}
	else
	{
		if (strcmp((*it).second.c_str(), version) != 0)
		{
			return;
		}
	}
	
	m_device.erase(it);
	
	if (m_handler != NULL)
	{
		m_handler->OnLostDevice(endpoint_reference);
	}
}
