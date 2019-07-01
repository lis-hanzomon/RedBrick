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

#ifndef SNAPSHOT_WEB_SERVER_H
#define SNAPSHOT_WEB_SERVER_H

#include "mhwebserver/MH_WebServer.h"
#include "mhmedia/MH_MediaPacket.h"
#include "mhmedia/MH_JpegWriter.h"
#include "mhengine/MH_CriticalSection.h"

#include <set>
#include <string>

class SnapShotWebServer : public MH_WebServer
{
private:
	unsigned short m_port_no;

	char m_path[255];
	std::set<std::string> m_profile_token;

	MH_CriticalSection m_sync;
	
protected:
	virtual bool OnRequest(HttpContext* ctx, const char* uri, const char* ipaddr);

public:
	SnapShotWebServer(const char* path);
	virtual ~SnapShotWebServer();

	void AddProfile(const char* profile_token);
	void RemoveProfile(const char* profile_token);

	bool UpdateSnapShot(const char* profile_token, const unsigned char* buff, int size);
};

#endif
