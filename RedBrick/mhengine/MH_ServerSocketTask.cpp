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
#include "mhengine/MH_ServerSocketTask.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_ServerSocketTask::MH_ServerSocketTask(EventHandler* handler)
	: m_handler(handler)
	, m_server_socket(NULL)
{
}

MH_ServerSocketTask::~MH_ServerSocketTask()
{
}

bool MH_ServerSocketTask::Open(MH_ServerSocket* server_socket, int backlog)
{
	m_server_socket = server_socket;

	if (!m_server_socket->Create(backlog))
	{
		return false;
	}

	Start();

	return true;
}

void MH_ServerSocketTask::Close()
{
	m_server_socket->Cancel();

	Stop();

	m_server_socket->Close();
}

void MH_ServerSocketTask::OnMain()
{
	while (!IsTerminate())
	{
		MH_Socket* client = m_server_socket->Accept();
		if(client != NULL)
		{
			m_handler->OnConnect(this, client);
		}
	}
}
