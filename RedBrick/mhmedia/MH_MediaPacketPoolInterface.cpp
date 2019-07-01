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
#include "mhmedia/MH_MediaPacketPoolInterface.h"
#include "mhmedia/MH_MediaPacket.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MediaPacketPoolInterface::MH_MediaPacketPoolInterface(const char* tag)
	: m_ref_count(0)
	, m_cancel(false)
{
	m_tag = strdup(tag);
}

MH_MediaPacketPoolInterface::~MH_MediaPacketPoolInterface()
{
	free(m_tag);
}

bool MH_MediaPacketPoolInterface::Create()
{
	if (m_ref_count != 0)
	{
		return false;
	}
	
	if (!m_event.Create())
	{
		return false;
	}

	m_cancel = false;
	
	m_ref_count = 1;
	
	return true;
}

void MH_MediaPacketPoolInterface::Close()
{
	if (m_ref_count == 0)
	{
		delete this;
		return;
	}
	
	{
		MH_AutoLock lock(&m_sync);
		
		m_ref_count--;
		if (0 < m_ref_count)
		{
			return;
		}
	}
	
	OnClose();
	m_event.Close();
	delete this;
}

void MH_MediaPacketPoolInterface::OnClose()
{
}

MH_MediaPacket* MH_MediaPacketPoolInterface::GetMediaPacket(int size, unsigned long timeout)
{
	MH_MediaPacket* media_packet = NULL;

	while (!m_cancel)
	{
		{
			MH_AutoLock lock(&m_sync);
			
			media_packet = OnGetMediaPacket(size);
			if (media_packet != NULL)
			{
				m_ref_count++;
				break;
			}
		}

		if (timeout == 0)
		{
			break;
		}

		if (!m_event.Wait(timeout))
		{
			break;
		}
	}

	return media_packet;
}

void MH_MediaPacketPoolInterface::Cancel()
{
	MH_AutoLock lock(&m_sync);
	
	m_cancel = true;
	m_event.Set();
}

void MH_MediaPacketPoolInterface::AddRef(MH_MediaPacket* media_packet)
{
	MH_AutoLock lock(&m_sync);

	media_packet->m_ref_count++;
}

void MH_MediaPacketPoolInterface::ReleaseRef(MH_MediaPacket* media_packet)
{
	{
		MH_AutoLock lock(&m_sync);

		media_packet->m_ref_count--;
		if (media_packet->m_ref_count == 0)
		{
			media_packet->Reset();

			OnReleasePacket(media_packet);
			
			m_ref_count--;
			if (0 < m_ref_count)
			{
				m_event.Set();
			}
		}
	}
	
	if (m_ref_count == 0)
	{
		OnClose();
		m_event.Close();
		delete this;
	}
}
