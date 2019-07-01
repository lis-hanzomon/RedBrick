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
#include "mhmedia/MH_MediaPacketSrc.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MediaPacketSrc::MH_MediaPacketSrc()
{
}

MH_MediaPacketSrc::~MH_MediaPacketSrc()
{
}

int MH_MediaPacketSrc::GetFrameRate()
{
	return 60;
}

int MH_MediaPacketSrc::GetBitrate()
{
	return 2000000;
}

int MH_MediaPacketSrc::GetMaxFrameSize()
{
	return 400000;
}

void MH_MediaPacketSrc::AddSink(MH_MediaPacketSink* sink)
{
	MH_AutoLock lock(&m_sync);
	m_sink.push_back(sink);
}

void MH_MediaPacketSrc::RemoveSink(MH_MediaPacketSink* sink)
{
	MH_AutoLock lock(&m_sync);
	m_sink.remove(sink);
}

void MH_MediaPacketSrc::Push(MH_MediaPacket* packet)
{
	MH_AutoLock lock(&m_sync);
	for(std::list<MH_MediaPacketSink*>::iterator it = m_sink.begin(); it != m_sink.end(); it++)
	{
		(*it)->Push(packet);
	}
}
