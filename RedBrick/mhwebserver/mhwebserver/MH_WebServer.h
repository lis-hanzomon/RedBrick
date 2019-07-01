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

#ifndef MH_WEB_SERVER_H
#define MH_WEB_SERVER_H

#include "mhengine/MH_Thread.h"

class MH_WebServer : public MH_Thread
{
public:
	struct HttpContext
	{
		void* conn;
		void* hm;
	};

private:
	unsigned short m_port_no;
	bool m_use_ipv6;

	static void stubEventHandler(void* nc, int ev, void* ev_data);
	void EventHandler(void* nc, int ev, void* ev_data);

protected:
	virtual void OnMain();

	virtual bool OnRequest(HttpContext* ctx, const char* uri, const char* ipaddr);

	bool SendFile(HttpContext* ctx, const char* content_type, const char* filename);
	void SetBinaryContent(HttpContext* ctx, int content_length = -1);
	void SendData(HttpContext* ctx, const unsigned char* data, int len);

public:
	MH_WebServer();
	virtual ~MH_WebServer();
	
	bool Start(unsigned short port_no, bool use_ipv6);
};

#endif
