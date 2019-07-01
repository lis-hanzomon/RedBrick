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
#include "SnapShotWebServer.h"

#include <unistd.h>

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

SnapShotWebServer::SnapShotWebServer(const char* path)
	: MH_WebServer()
{
	strcpy(m_path, path);
}

SnapShotWebServer::~SnapShotWebServer()
{
}

void SnapShotWebServer::AddProfile(const char* profile_token)
{
	MH_AutoLock lock(&m_sync);
	m_profile_token.insert(profile_token);
}

void SnapShotWebServer::RemoveProfile(const char* profile_token)
{
	MH_AutoLock lock(&m_sync);
	m_profile_token.erase(profile_token);
	
	char path[256];
	sprintf(path, "%s/%s.jpg", m_path, profile_token);
	unlink(path);
}

bool SnapShotWebServer::OnRequest(HttpContext* ctx, const char* uri, const char* ipaddr)
{
	MH_AutoLock lock(&m_sync);

	//---------------------------
	// uri : /[profile_token].jpg
	//---------------------------
	
	int len = strlen(uri);
	if (len < 6 || 128 < len)
	{
		return false;
	}
	
	char profile_token[256];
	strncpy(profile_token, uri + 1, len - 5);
	*(profile_token + len - 5) = 0x00;

	if (m_profile_token.find(profile_token) != m_profile_token.end())
	{
		char path[256];
		sprintf(path, "%s%s", m_path, uri);
		SendFile(ctx, "image/jpeg", path);
	}
	else
	{
		return false;
	}
	
	return true;
}

bool SnapShotWebServer::UpdateSnapShot(const char* profile_token, const unsigned char* buff, int size)
{
	MH_AutoLock lock(&m_sync);
	
	if (m_profile_token.find(profile_token) != m_profile_token.end())
	{
		char path[256];
		sprintf(path, "%s/%s.jpg", m_path, profile_token);
		// DBG_PRINT("### Update : %s ###", path);

		FILE* fp = fopen(path, "w");
		if (fp == NULL)
		{
			DBG_PRINT("Error: fopen(%s)", path);
			return false;
		}
		fwrite(buff, 1, size, fp);
		fclose(fp);
	}
	
	return true;
}
