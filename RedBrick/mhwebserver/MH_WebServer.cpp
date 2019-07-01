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
#include "mhwebserver/MH_WebServer.h"
#include "mongoose.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_WebServer::MH_WebServer()
{
}

MH_WebServer::~MH_WebServer()
{
}

bool MH_WebServer::Start(unsigned short port_no, bool use_ipv6)
{
	m_port_no  = port_no;
	m_use_ipv6 = use_ipv6;
	
	return MH_Thread::Start();
}

void MH_WebServer::OnMain()
{
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, NULL);

	char s_http_port[20];
	
	if (!m_use_ipv6)
	{
		// ipv4
		sprintf(s_http_port, "%d", m_port_no);
	}
	else
	{
		// ipv6
		sprintf(s_http_port, "[::]:%d", m_port_no);
	}

	struct mg_bind_opts opts;
	memset(&opts, 0, sizeof(opts));
	opts.user_data = this;
	struct mg_connection* nc = mg_bind_opt(&mgr, s_http_port, (mg_event_handler_t)stubEventHandler, opts);
	if (nc == NULL)
	{
		return;
	}

	mg_set_protocol_http_websocket(nc);

	while(!IsTerminate())
	{
		mg_mgr_poll(&mgr, 1000);
	}
	
	mg_mgr_free(&mgr);
}

void MH_WebServer::stubEventHandler(void* nc, int ev, void* ev_data)
{
	MH_WebServer* own = (MH_WebServer*)(((struct mg_connection*)nc)->user_data);
	own->EventHandler(nc, ev, ev_data);
}

static void mg_vcpy(const struct mg_str *str1, char *str2, size_t n2)
{
	size_t n1 = str1->len;
	size_t n = str1->len < (n2 - 1) ? str1->len : (n2 - 1);
	strncpy(str2, str1->p, n);
	*(str2 + n) = 0x00;
}

void MH_WebServer::EventHandler(void* nc, int ev, void* ev_data)
{
	struct http_message* hm = (struct http_message *)ev_data;
	if (ev == MG_EV_HTTP_REQUEST)
  	{
		char uri[255];
		mg_vcpy(&(hm->uri), uri, sizeof(uri));
		
  		mg_connection* conn = (mg_connection*)nc;
		char ipaddr[255];
		mg_sock_addr_to_str(&(conn->sa), ipaddr, sizeof(ipaddr), MG_SOCK_STRINGIFY_IP);
  		
		HttpContext ctx;
		ctx.conn = nc;
		ctx.hm = hm;

		OnRequest(&ctx, uri, ipaddr);
	}
}

bool MH_WebServer::OnRequest(HttpContext* ctx, const char* uri, const char* ipaddr)
{
	return false;
}

bool MH_WebServer::SendFile(HttpContext* ctx, const char* content_type, const char* filepath)
{
	int len = strlen(filepath);
	if (255 < len)
	{
		return false;
	}
	
	FILE* fp = fopen(filepath, "r");
	if (fp == NULL)
	{
		return false;
	}

	fseek(fp, 0, SEEK_END);
	int content_length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned char* buff = new unsigned char[content_length];
	fread(buff, 1, content_length, fp);
	fclose(fp);

	char filename[256];
	const char* pos = strrchr(filepath, '/');
	if (pos != NULL)
	{
		strcpy(filename, pos + 1);
	}
	else
	{
		strcpy(filename, filepath);
	}

	char header[512];
	sprintf(header, "Content-Type: %s; charset=utf-8;\r\nCache-Control: no-store\r\ninline; filename=%s", content_type, filename);

	mg_send_head((struct mg_connection*)ctx->conn, 200, content_length, header);
	mg_send((struct mg_connection*)ctx->conn, buff, content_length);

	delete [] buff;

	return true;
}

void MH_WebServer::SetBinaryContent(HttpContext* ctx, int content_length)
{
	mg_send_head((struct mg_connection*)ctx->conn, 200, content_length, "Content-Type: application/octet-stream\r\nCache-Control: no-store");
}

void MH_WebServer::SendData(HttpContext* ctx, const unsigned char* data, int len)
{
	mg_send((struct mg_connection*)ctx->conn, data, len);
}
