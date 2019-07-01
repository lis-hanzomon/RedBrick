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

#ifndef MH_VIDEO_SERVER_MEDIA_SUBSESSION_H
#define MH_VIDEO_SERVER_MEDIA_SUBSESSION_H

#include <OnDemandServerMediaSubsession.hh>

#include "mhmedia/MH_MediaPacketSrc.h"

class MH_VideoServerMediaSubsession : public OnDemandServerMediaSubsession
{
public:
	static MH_VideoServerMediaSubsession* createNew(UsageEnvironment& env, MH_MediaPacketSrc* src, int max_store_sec);

private:
	MH_MediaPacketSrc* m_src;
	int m_max_store_sec;

	MH_VideoServerMediaSubsession(UsageEnvironment& env, MH_MediaPacketSrc* src, int max_store_sec);
	virtual ~MH_VideoServerMediaSubsession();

	virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);
};

#endif
