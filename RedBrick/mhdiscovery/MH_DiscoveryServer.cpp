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
#include "mhdiscovery/MH_DiscoveryServer.h"
#include "MH_DiscoveryParser.h"

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

MH_DiscoveryServer::MH_DiscoveryServer()
	: m_discovery_thread(NULL)
	, m_endpoint_reference(NULL)
	, m_type_ns(NULL)
	, m_type(NULL)
	, m_scope(NULL)
	, m_xaddr(NULL)
	, m_version(NULL)
	, m_instance_id(0)
	, m_message_number(0)
	, m_socket(NULL)
{
}

MH_DiscoveryServer::~MH_DiscoveryServer()
{
}

bool MH_DiscoveryServer::Start(const char* net_interface, const char* endpoint_reference, const char* type_ns, const char* type, const char* scope, const char* xaddr, const char* version)
{
	if (net_interface == NULL || endpoint_reference == NULL || type_ns == NULL || type == NULL || scope == NULL || xaddr == NULL || version == NULL)
	{
		return false;
	}
	if (m_socket != NULL)
	{
		return false;
	}
	
	m_endpoint_reference = strdup(endpoint_reference);
	m_type_ns = strdup(type_ns);
	m_type    = strdup(type);
	m_scope   = strdup(scope);
	m_xaddr   = strdup(xaddr);
	m_version = strdup(version);
	
	m_instance_id++;
	
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

	m_discovery_thread = new MH_DiscoveryThread(this);
	if (!m_discovery_thread->Start(net_interface))
	{
		Stop();
		return false;
	}

	SendHello();
	
	return true;
}

void MH_DiscoveryServer::Stop()
{
	if (m_discovery_thread != NULL)
	{
		m_discovery_thread->Stop();
		delete m_discovery_thread;
		m_discovery_thread = NULL;
	}
	
	SendBye();
	
	if (m_socket != NULL)
	{
		m_socket->Close();
		delete m_socket;
		m_socket = NULL;
	}
	
	if (m_endpoint_reference != NULL)
	{
		free(m_endpoint_reference);
		m_endpoint_reference = NULL;
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
	if (m_scope != NULL)
	{
		free(m_scope);
		m_scope = NULL;
	}
	if (m_xaddr != NULL)
	{
		free(m_xaddr);
		m_xaddr = NULL;
	}
	if (m_version != NULL)
	{
		free(m_version);
		m_version = NULL;
	}
}

void MH_DiscoveryServer::OnRecvMessage(sockaddr_in* addr, const char* message)
{
	MSG_DATA msg_data;
    memset(&msg_data, 0x00, sizeof(msg_data));
	
	if (MH_DiscoveryParser::Parser(message, strlen(message), &msg_data))
	{
		if (msg_data.action_type == ACTION_TYPE_PROBE)
		{
			if (MH_DiscoveryParser::CheckTypes(msg_data.body_types, m_type_ns, m_type))
			{
				SendProbeMatch(addr, msg_data.header_message_id);
			}
		}
	}
}

void MH_DiscoveryServer::SendHello()
{
	if (m_socket == NULL)
	{
		return;
	}
	
	m_message_number++;
	
	char message_id[40];
	UuidGenerate(message_id);
	
	char cmd[2048];
	sprintf(
		cmd, 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
			"<s:Header>"
				"<a:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</a:To>"
				"<a:MessageID>uuid:%s</a:MessageID>"
				"<a:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello</a:Action>"
				"<d:AppSequence MessageNumber=\"%d\" InstanceId=\"%d\"></d:AppSequence>"
			"</s:Header>"
			"<s:Body>"
				"<d:Hello>"
					"<a:EndpointReference><a:Address>urn:uuid:%s</a:Address></a:EndpointReference>"
					"<d:Types xmlns:mh=\"%s\">mh:%s</d:Types>"
					"<d:Scopes MatchBy=\"http://schemas.xmlsoap.org/ws/2005/04/discovery/rfc3986\">%s</d:Scopes>"
					"<d:XAddrs>%s</d:XAddrs>"
					"<d:MetadataVersion>%s</d:MetadataVersion>"
				"</d:Hello>"
			"</s:Body>"
		"</s:Envelope>",
		message_id,
		m_message_number,
		m_instance_id,
		m_endpoint_reference,
		m_type_ns,
		m_type,
		m_scope,
		m_xaddr,
		m_version
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

void MH_DiscoveryServer::SendBye()
{
	if (m_socket == NULL)
	{
		return;
	}
	
	m_message_number++;
	
	char message_id[40];
	UuidGenerate(message_id);
	
	char cmd[2048];
	sprintf(
		cmd, 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
			"<s:Header>"
				"<a:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</a:To>"
				"<a:MessageID>uuid:%s</a:MessageID>"
				"<a:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye</a:Action>"
				"<d:AppSequence MessageNumber=\"%d\" InstanceId=\"%d\"></d:AppSequence>"
			"</s:Header>"
			"<s:Body>"
				"<d:Bye>"
					"<a:EndpointReference><a:Address>urn:uuid:%s</a:Address></a:EndpointReference>"
					"<d:Types xmlns:mh=\"%s\">mh:%s</d:Types>"
					"<d:Scopes MatchBy=\"http://schemas.xmlsoap.org/ws/2005/04/discovery/rfc3986\">%s</d:Scopes>"
					"<d:XAddrs>%s</d:XAddrs>"
					"<d:MetadataVersion>%s</d:MetadataVersion>"
				"</d:Bye>"
			"</s:Body>"
		"</s:Envelope>",
		message_id,
		m_message_number,
		m_instance_id,
		m_endpoint_reference,
		m_type_ns,
		m_type,
		m_scope,
		m_xaddr,
		m_version
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

void MH_DiscoveryServer::SendProbeMatch(sockaddr_in* from, const char* relate_message_id)
{
	m_message_number++;
	
	char message_id[40];
	UuidGenerate(message_id);
	
	char cmd[2048];
	sprintf(
		cmd, 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
			"<s:Header>"
				"<a:MessageID>uuid:%s</a:MessageID>"
				"<a:RelatesTo>%s</a:RelatesTo>"
				"<a:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</a:Action>"
				"<d:AppSequence MessageNumber=\"%d\" InstanceId=\"%d\"></d:AppSequence>"
			"</s:Header>"
			"<s:Body>"
				"<d:ProbeMatches>"
					"<d:ProbeMatch>"
						"<a:EndpointReference><a:Address>urn:uuid:%s</a:Address></a:EndpointReference>"
						"<d:Types xmlns:mh=\"%s\">mh:%s</d:Types>"
						"<d:Scopes>%s</d:Scopes>"
						"<d:XAddrs>%s</d:XAddrs>"
						"<d:MetadataVersion>%s</d:MetadataVersion>"
					"</d:ProbeMatch>"
				"</d:ProbeMatches>"
			"</s:Body>"
		"</s:Envelope>",
		message_id,
		relate_message_id,
		m_message_number,
		m_instance_id,
		m_endpoint_reference,
		m_type_ns,
		m_type,
		m_scope,
		m_xaddr,
		m_version
	);
	
	MH_UdpSocket* socket = new MH_UdpSocket(false);
	socket->Create();
	
	struct sockaddr_in addr;
	addr.sin_family      = AF_INET;
	addr.sin_port        = from->sin_port;
 	addr.sin_addr.s_addr = from->sin_addr.s_addr;

	socket->SendTo(cmd, strlen(cmd), &addr, sizeof(addr));
	
	socket->Close();
	delete socket;
}
