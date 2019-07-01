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
#include "mhmedia/MH_MediaPacketPool.h"
#include "mhmedia/MH_MediaPacket.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MediaPacketPool::MH_MediaPacketPool(const char* tag)
	: MH_MediaPacketPoolInterface(tag)
	, m_size(0)
	, m_expand(false)
	, m_buffer(NULL)
	, m_media_packets()
{
}

MH_MediaPacketPool::~MH_MediaPacketPool()
{
}

bool MH_MediaPacketPool::Create(int size, int count, bool expand)
{
	if (size <= 0 || count <= 0)
	{
		return false;
	}
	
	if (!MH_MediaPacketPoolInterface::Create())
	{
		return false;
	}

	m_size = size;
	m_expand = expand;

	int buffer_size = size * count;

	m_buffer = new unsigned char[buffer_size];

	for (int i = 0; i < count; i++)
	{
		MH_MediaPacket* packet = new MH_MediaPacket(this, m_buffer + size * i, size);
		m_media_packets.push(packet);
	}
	
	return true;
}

MH_MediaPacket* MH_MediaPacketPool::OnGetMediaPacket(int size)
{
	// DBG_PRINT("Enter MH_MediaPacketPool::OnGetMediaPacket");

	if (size <= 0)
	{
		size = m_size;
	}
	
	MH_MediaPacket* media_packet = NULL;

	if (!m_media_packets.empty())
	{
		media_packet = m_media_packets.front();
		m_media_packets.pop();

		if (media_packet->m_buffer_size < size)
		{
			DBG_PRINT("Warning: Expand Memory : %d -> %d", media_packet->m_buffer_size, size);
			media_packet->Allocate(size);
		}

		media_packet->AddRef();
	}
	else
	{
		if (m_expand)
		{
			media_packet = new MH_MediaPacket(this);
			media_packet->Allocate(size);
			media_packet->AddRef();
		}
	}

	// DBG_PRINT("Exit MH_MediaPacketPool::OnGetMediaPacket");

	return media_packet;
}

void MH_MediaPacketPool::OnReleasePacket(MH_MediaPacket* media_packet)
{
	m_media_packets.push(media_packet);
	// DBG_PRINT("m_media_packets size = %d", m_media_packets.size());
}

void MH_MediaPacketPool::OnClose()
{
	while (!m_media_packets.empty())
	{
		MH_MediaPacket* media_packet = m_media_packets.front();
		m_media_packets.pop();
		delete media_packet;
	}
	
	delete[] m_buffer;
	m_buffer = NULL;
}
