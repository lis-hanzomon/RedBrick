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

#ifndef MH_MEDIA_PACKET_POOL_INTERFACE_H
#define MH_MEDIA_PACKET_POOL_INTERFACE_H

#include "MH_Media.h"

#include "mhengine/MH_Event.h"
#include "mhengine/MH_CriticalSection.h"

class MH_MediaPacket;

class MHMEDIA_API MH_MediaPacketPoolInterface
{
private:
	char* m_tag;
	MH_CriticalSection m_sync;
	int m_ref_count;
	
	void AddRef(MH_MediaPacket* media_packet);
	void ReleaseRef(MH_MediaPacket* media_packet);

protected:
	MH_Event m_event;
	bool m_cancel;

	virtual ~MH_MediaPacketPoolInterface();

	virtual MH_MediaPacket* OnGetMediaPacket(int size) = 0;
	virtual void OnReleasePacket(MH_MediaPacket* media_packet) = 0;
	virtual void OnClose();

public:
	MH_MediaPacketPoolInterface(const char* tag);

	bool Create();
	void Cancel();
	void Close();

	MH_MediaPacket* GetMediaPacket(int size = 0, unsigned long timeout = -1);

	friend class MH_MediaPacket;
};

#endif
