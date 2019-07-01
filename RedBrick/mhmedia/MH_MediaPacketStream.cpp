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
#include "mhmedia/MH_MediaPacketStream.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MediaPacketStream::MH_MediaPacketStream(const char* tag)
	: m_tag(NULL)
	, m_max_store_num(-1)
	, m_cancel(false)
	, m_error(false)
{
	DBG_TRACE("Enter MH_MediaPacketStream::MH_MediaPacketStream");
	DBG_PRINT("tag = %s", tag);

	m_tag = strdup(tag);

	DBG_TRACE("Exit MH_MediaPacketStream::MH_MediaPacketStream");
}

MH_MediaPacketStream::~MH_MediaPacketStream()
{
	DBG_TRACE("Enter MH_MediaPacketStream::MH_MediaPacketStream");

	free(m_tag);

	DBG_TRACE("Exit MH_MediaPacketStream::~MH_MediaPacketStream");
}

bool MH_MediaPacketStream::Open(int max_store_num)
{
	DBG_TRACE("Enter MH_MediaPacketStream::Open");

	if (!m_event.Create())
	{
		return false;
	}

	m_max_store_num = max_store_num;
	m_cancel = false;
	m_error = false;

	DBG_TRACE("Exit MH_MediaPacketStream::Open");

	return true;
}

void MH_MediaPacketStream::Close()
{
	DBG_TRACE("Enter MH_MediaPacketStream::Close");

	while (!m_queue.empty())
	{
		MH_MediaPacket* media_packet = m_queue.front();
		m_queue.pop();

		media_packet->ReleaseRef();
	}

	m_event.Close();

	DBG_TRACE("Exit MH_MediaPacketStream::Close");
}

void MH_MediaPacketStream::Cancel()
{
	DBG_TRACE("Enter MH_MediaPacketStream::Cancel");

	{
		MH_AutoLock lock(&m_sync);

		while (!m_queue.empty())
		{
			MH_MediaPacket* media_packet = m_queue.front();
			m_queue.pop();

			media_packet->ReleaseRef();
		}

		m_cancel = true;
		m_event.Set();
	}

	DBG_TRACE("Exit MH_MediaPacketStream::Cancel");
}

void MH_MediaPacketStream::Error()
{
	DBG_TRACE("Enter MH_MediaPacketStream::Error");

	{
		MH_AutoLock lock(&m_sync);

		m_error = true;
		m_event.Set();
	}

	DBG_TRACE("Exit MH_MediaPacketStream::Error");
}

bool MH_MediaPacketStream::IsError()
{
	MH_AutoLock lock(&m_sync);

	return m_error;
}

void MH_MediaPacketStream::Push(MH_MediaPacket* media_packet)
{
	// DBG_TRACE("Enter MH_MediaPacketStream::Push");
	// DBG_PRINT("m_tag = %s", m_tag);

	{
		MH_AutoLock lock(&m_sync);

		if (m_cancel)
		{
			return;
		}
		
		if (m_max_store_num != -1 && m_max_store_num < m_queue.size())
		{
			int half_size = m_max_store_num / 2;
			while(half_size < m_queue.size())
			{
				media_packet = m_queue.front();
				m_queue.pop();
				
				media_packet->ReleaseRef();
			}
		}
		
		media_packet->AddRef();

		m_queue.push(media_packet);
		// DBG_PRINT("queue size = %d", m_queue.size());
		
		m_event.Set();
	}

	// DBG_TRACE("Exit MH_MediaPacketStream::Push");
}

MH_MediaPacket* MH_MediaPacketStream::Pop(long timeout)
{
	// DBG_TRACE("Enter MH_MediaPacketStream::Pop");
	// DBG_PRINT("m_tag = %s", m_tag);

	MH_MediaPacket* media_packet = NULL;

	while (!m_cancel)
	{
		{
			MH_AutoLock lock(&m_sync);

			if (!m_queue.empty())
			{
				media_packet = m_queue.front();
				m_queue.pop();
			}
		}

		if (media_packet != NULL)
		{
			break;
		}

		if (m_error)
		{
			break;
		}

		// DBG_PRINT("+++ WAIT +++");
		if (!m_event.Wait(timeout))
		{
			break;
		}
		// DBG_PRINT("+++ WAIT COMP +++");
	}

	// DBG_TRACE("Exit MH_MediaPacketStream::Pop");

	return media_packet;
}

bool MH_MediaPacketStream::IsEmpty()
{
	MH_AutoLock lock(&m_sync);

	bool ret = m_queue.empty();

	return ret;
}

int MH_MediaPacketStream::GetCount()
{
	MH_AutoLock lock(&m_sync);

	int ret = m_queue.size();

	return ret;
}

void MH_MediaPacketStream::Clear()
{
	DBG_TRACE("Enter MH_MediaPacketStream::Clear");
	DBG_PRINT("m_tag = %s", m_tag);

	MH_AutoLock lock(&m_sync);

	while (m_queue.size())
	{
		MH_MediaPacket* media_packet = m_queue.front();
		m_queue.pop();

		DBG_PRINT("pts = %lld", media_packet->m_pts);

		media_packet->ReleaseRef();
	}

	DBG_TRACE("Exit MH_MediaPacketStream::Clear");
}
