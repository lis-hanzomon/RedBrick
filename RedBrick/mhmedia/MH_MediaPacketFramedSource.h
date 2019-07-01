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

#ifndef MH_MEDIA_PACKET_FRAMED_SOURCE_H
#define MH_MEDIA_PACKET_FRAMED_SOURCE_H

#include <MediaSink.hh>

#include "mhmedia/MH_MediaPacketSrc.h"
#include "mhmedia/MH_MediaPacketSink.h"
#include "mhmedia/MH_MediaPacketStream.h"

class MH_MediaPacketFramedSource : public FramedSource
								 , public MH_MediaPacketSink
{
public:
	static MH_MediaPacketFramedSource* createNew(UsageEnvironment& env, MH_MediaPacketSrc* src, int max_store_sec);
  
private:
	MH_CriticalSection m_sync;
	MH_MediaPacketSrc* m_src;
	MH_MediaPacketStream* m_stream;
	bool m_recieved_key_frame;
	bool m_is_play;
	bool m_req_next_frame;

	MH_MediaPacketFramedSource(UsageEnvironment& env, MH_MediaPacketSrc* src);
	virtual ~MH_MediaPacketFramedSource();
  
	bool Open(int max_store_sec);
	void Close();

	virtual void doGetNextFrame();
	virtual void doStopGettingFrames();

	virtual void Push(MH_MediaPacket* packet);
	
	void NotifyPacket();

public:
	virtual unsigned maxFrameSize() const;
};

#endif
