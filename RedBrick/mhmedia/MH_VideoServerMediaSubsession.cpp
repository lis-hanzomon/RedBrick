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
#include "MH_VideoServerMediaSubsession.h"
#include "MH_MediaPacketFramedSource.h"
#include <H264VideoStreamDiscreteFramer.hh>
#include <H264VideoRTPSink.hh>

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_VideoServerMediaSubsession* MH_VideoServerMediaSubsession::createNew(UsageEnvironment& env, MH_MediaPacketSrc* src, int max_store_sec)
{
	return new MH_VideoServerMediaSubsession(env, src, max_store_sec);
}

MH_VideoServerMediaSubsession::MH_VideoServerMediaSubsession(UsageEnvironment& env, MH_MediaPacketSrc* src, int max_store_sec)
	: OnDemandServerMediaSubsession(env, True)
	, m_src(src)
	, m_max_store_sec(max_store_sec)
{
}

MH_VideoServerMediaSubsession::~MH_VideoServerMediaSubsession()
{
}

FramedSource* MH_VideoServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
	
	FramedSource* source = MH_MediaPacketFramedSource::createNew(envir(), m_src, m_max_store_sec);
	
	H264VideoStreamDiscreteFramer* videoSource = H264VideoStreamDiscreteFramer::createNew(envir(), source);
 	
	estBitrate = m_src->GetBitrate() / 1000;	// kbps
	
	return videoSource;
}

RTPSink* MH_VideoServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
	OutPacketBuffer::maxSize = m_src->GetMaxFrameSize();
	
	return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
