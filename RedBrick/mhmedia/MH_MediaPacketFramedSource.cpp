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
#include "MH_MediaPacketFramedSource.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_MediaPacketFramedSource* MH_MediaPacketFramedSource::createNew(UsageEnvironment& env, MH_MediaPacketSrc* src, int max_store_sec)
{
	MH_MediaPacketFramedSource* newSource = new MH_MediaPacketFramedSource(env, src);
	if (newSource == NULL)
	{
		return NULL;
	}
	
	if (!newSource->Open(max_store_sec))
	{
		delete newSource;
		return NULL;
	}
	
	return newSource;
}

MH_MediaPacketFramedSource::MH_MediaPacketFramedSource(UsageEnvironment& env, MH_MediaPacketSrc* src)
	: FramedSource(env)
	, m_src(src)
	, m_stream(NULL)
	, m_recieved_key_frame(false)
	, m_is_play(false)
	, m_req_next_frame(false)
{
}

MH_MediaPacketFramedSource::~MH_MediaPacketFramedSource()
{
	Close();
}

unsigned MH_MediaPacketFramedSource::maxFrameSize() const
{
	return m_src->GetMaxFrameSize();
}

bool MH_MediaPacketFramedSource::Open(int max_store_sec)
{
	m_stream = new MH_MediaPacketStream("MH_MediaPacketFramedSource");
	if (!m_stream->Open(m_src->GetFrameRate() * max_store_sec))
	{
		return false;
	}
	
	return true;
}

void MH_MediaPacketFramedSource::Close()
{
	if (m_stream != NULL)
	{
		m_stream->Clear();
		delete m_stream;
		m_stream = NULL;
	}
}

void MH_MediaPacketFramedSource::doGetNextFrame()
{
	MH_AutoLock lock(&m_sync);
	
	if (!m_is_play)
	{
		m_is_play        = true;
		m_req_next_frame = true;
		m_src->AddSink(this);
	}
	else
	{
		if (!m_stream->IsEmpty())
		{
			MH_MediaPacket* packet = m_stream->Pop();
			
			if (packet->m_size <= fMaxSize)
			{
				memcpy(fTo, packet->m_data, packet->m_size);
				fFrameSize                = packet->m_size;
				fNumTruncatedBytes        = 0;
			}
			else
			{
				DBG_PRINT("Warnning: size over - %d < %d", fMaxSize, packet->m_size);
				
				memcpy(fTo, packet->m_data, fMaxSize);
				fFrameSize                = fMaxSize;
				fNumTruncatedBytes        = packet->m_size - fMaxSize;
			}
			
			fPresentationTime.tv_sec  = packet->m_pts / 1000000;
			fPresentationTime.tv_usec = packet->m_pts % 1000000;
			fDurationInMicroseconds   = 0;
			
			packet->ReleaseRef();
			
			afterGetting(this);
		}
		else
		{
			m_req_next_frame = true;
		}
	}
}

void MH_MediaPacketFramedSource::doStopGettingFrames()
{
	if (m_is_play)
	{
		MH_AutoLock lock(&m_sync);
		
		m_src->RemoveSink(this);
		m_is_play        = false;
		m_req_next_frame = false;
	}
	
	// DBG_TRACE("Exit MH_MediaPacketFramedSource::doStopGettingFrames");
}

void MH_MediaPacketFramedSource::Push(MH_MediaPacket* packet)
{
	// DBG_TRACE("Enter MH_MediaPacketFramedSource::Push");
	
	MH_AutoLock lock(&m_sync);
	
	if(!m_is_play)
	{
		DBG_PRINT("Warnning: m_is_play = false");
		return;
	}
	
	if (!m_recieved_key_frame)
	{
		if (packet->m_key_frame)
		{
			m_recieved_key_frame = true;
		}
	}
	
	if (m_recieved_key_frame)
	{
		// DBG_PRINT("PUSH: key = %d, size = %d, pts = %lld", packet->m_key_frame, packet->m_size, packet->m_pts);
		m_stream->Push(packet);

		if(m_req_next_frame)
		{
			m_req_next_frame = false;
	
			MH_MediaPacket* packet = m_stream->Pop();
			
			if (packet->m_size <= fMaxSize)
			{
				memcpy(fTo, packet->m_data, packet->m_size);
				fFrameSize                = packet->m_size;
				fNumTruncatedBytes        = 0;
			}
			else
			{
				DBG_PRINT("Warnning: size over - %d < %d", fMaxSize, packet->m_size);
				
				memcpy(fTo, packet->m_data, fMaxSize);
				fFrameSize                = fMaxSize;
				fNumTruncatedBytes        = packet->m_size - fMaxSize;
			}
			
			fPresentationTime.tv_sec  = packet->m_pts / 1000000;
			fPresentationTime.tv_usec = packet->m_pts % 1000000;
			fDurationInMicroseconds   = 0;
			
			packet->ReleaseRef();
	
			nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
									 (TaskFunc*)FramedSource::afterGetting, this);
		}
	}
}
