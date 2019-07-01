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
#include "mhmedia/MH_MediaPacket.h"
#include "mhmedia/MH_MediaPacketPoolInterface.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MediaPacket::MH_MediaPacket(MH_MediaPacketPoolInterface* pool)
	: m_pool(pool)
	, m_ref_count(0)
	, m_expand(true)
	, m_id(0)
	, m_type(0)
	, m_key_frame(false)
	, m_pts(PTS_NO_VALUE)
	, m_dts(PTS_NO_VALUE)
	, m_buffer(NULL)
	, m_buffer_size(0)
	, m_data(NULL)
	, m_size(0)
	, m_duration(-1)
{
}

MH_MediaPacket::MH_MediaPacket(MH_MediaPacketPoolInterface* pool, unsigned char* data, int data_size)
	: m_pool(pool)
	, m_ref_count(0)
	, m_expand(false)
	, m_id(0)
	, m_type(0)
	, m_key_frame(false)
	, m_pts(-1)
	, m_dts(-1)
	, m_buffer(data)
	, m_buffer_size(data_size)
	, m_data(data)
	, m_size(0)
	, m_duration(-1)
{
}

MH_MediaPacket::~MH_MediaPacket()
{
	Free();
}

bool MH_MediaPacket::Allocate(int buffer_size)
{
	Free();

	m_buffer = new unsigned char[buffer_size];
	if (m_buffer == NULL)
	{
		return false;
	}
	m_buffer_size = buffer_size;
	
	m_data = m_buffer;
	
	m_expand = true;

	return true;
}

void MH_MediaPacket::Free()
{
	if (m_expand)
	{
		delete[] m_buffer;
		m_buffer = NULL;
	}
	m_data = NULL;
}

void MH_MediaPacket::Reset()
{
	m_id        = 0;
	m_type      = 0;
	m_key_frame = false;
	m_pts       = PTS_NO_VALUE;
	m_dts       = PTS_NO_VALUE;
	m_size      = 0;
	m_duration  = -1;
}

void MH_MediaPacket::AddRef()
{
	if (m_pool == NULL)
	{
		return;
	}
	m_pool->AddRef(this);
}

void MH_MediaPacket::ReleaseRef()
{
	if (m_pool == NULL)
	{
		return;
	}
	m_pool->ReleaseRef(this);
}

bool MH_MediaPacket::Store(unsigned char* data, int data_size)
{
	if (m_buffer_size < data_size)
	{
		DBG_PRINT("MH_MediaPacket::Store : Invalid Size %d < %d", m_buffer_size, data_size);
		return false;
	}
	
	memcpy(m_buffer, data, data_size);
	m_size = data_size;
	
	return true;
}
