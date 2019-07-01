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
#include "mhmedia/MH_RtspServerThread.h"
#include "MH_VideoServerMediaSubsession.h"

#include <BasicUsageEnvironment.hh>

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_RtspServerThread::MH_RtspServerThread()
	: m_src(NULL)
	, m_use_auth(true)
	, m_port(0)
	, m_eventLoopWatchVariable(0)
	, m_authDB(NULL)
{
}

MH_RtspServerThread::~MH_RtspServerThread()
{
}

bool MH_RtspServerThread::Create()
{
	m_authDB = new UserAuthenticationDatabase;
	
	return true;
}

void MH_RtspServerThread::Destroy()
{
	if (m_authDB != NULL)
	{
		delete m_authDB;
		m_authDB = NULL;
	}
}

void MH_RtspServerThread::AddUser(const char* username, const char* password)
{
    m_authDB->addUserRecord(username, password);
}

void MH_RtspServerThread::RemoveUser(const char* username)
{
	m_authDB->removeUserRecord(username);
}

bool MH_RtspServerThread::Start(MH_MediaPacketSrc* src, const char* session_name, short port, int max_store_sec, bool use_auth)
{
	m_src = src;
	m_max_store_sec = max_store_sec;
	
	strcpy(m_session_name, session_name);
	
	m_port                   = port;
	m_use_auth               = use_auth;
	m_eventLoopWatchVariable = 0;
	
	return MH_Thread::Start();
}

void MH_RtspServerThread::OnMain()
{
	portNumBits rtspServerPortNum = m_port;
	
  	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
	
	RTSPServer* rtspServer = NULL;
	if (m_use_auth)
	{
		rtspServer = RTSPServer::createNew(*env, rtspServerPortNum, m_authDB);
	}
	else
	{
		rtspServer = RTSPServer::createNew(*env, rtspServerPortNum, NULL);
	}
    if (rtspServer != NULL) 
    {
	    ServerMediaSession* sms = ServerMediaSession::createNew(
	        *env, 
	        m_session_name, 
	        NULL, 
	        "RTSP/RTP stream",
	        false
		);
	    rtspServer->addServerMediaSession(sms);
		sms->addSubsession(MH_VideoServerMediaSubsession::createNew(sms->envir(), m_src, m_max_store_sec));
		
		env->taskScheduler().doEventLoop(&m_eventLoopWatchVariable);
	  
	  	Medium::close(rtspServer);
	}
	
	env->reclaim();
	delete scheduler;
}

void MH_RtspServerThread::OnReqTerminate()
{
	m_eventLoopWatchVariable = 1;
}
