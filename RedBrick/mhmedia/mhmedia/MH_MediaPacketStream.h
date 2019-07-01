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

#ifndef MH_MEDIA_PACKET_STREAM_H
#define MH_MEDIA_PACKET_STREAM_H

#include "MH_Media.h"

#include "mhengine/MH_CriticalSection.h"
#include "mhengine/MH_Event.h"
#include "MH_MediaPacket.h"

#include <queue>

class MHMEDIA_API MH_MediaPacketStream
{
private:
	char* m_tag;

	MH_CriticalSection m_sync;
	std::queue<MH_MediaPacket*> m_queue;

	int m_max_store_num;
	
	MH_Event m_event;
	bool m_cancel;
	bool m_error;

public:
	MH_MediaPacketStream(const char* tag);
	virtual ~MH_MediaPacketStream();

	bool Open(int max_store_num = -1);
	void Close();

	void Cancel();
	void Error();

	bool IsError();

	void Push(MH_MediaPacket* media_packet);
	MH_MediaPacket* Pop(long timeout = -1);

	bool IsEmpty();
	
	int GetCount();
	void Clear();
};

#endif
