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

#ifndef MH_MEDIA_PACKET_SRC_H
#define MH_MEDIA_PACKET_SRC_H

#include "MH_Media.h"

#include "mhengine/MH_CriticalSection.h"
#include "MH_MediaPacket.h"
#include "MH_MediaPacketSink.h"

#include <list>

class MHMEDIA_API MH_MediaPacketSrc
{
private:
	unsigned char* m_extra_data;
	int m_extra_data_size;

	MH_CriticalSection m_sync;
	std::list<MH_MediaPacketSink*> m_sink;

protected:
	void Push(MH_MediaPacket* packet);

public:
	MH_MediaPacketSrc();
	virtual ~MH_MediaPacketSrc();
	
	virtual int GetFrameRate();
	virtual int GetBitrate();
	virtual int GetMaxFrameSize();
	
	void AddSink(MH_MediaPacketSink* sink);
	void RemoveSink(MH_MediaPacketSink* sink);
};

#endif
