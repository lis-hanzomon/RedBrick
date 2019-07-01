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

#ifndef MH_MEDIA_PACKET_POOL_H
#define MH_MEDIA_PACKET_POOL_H

#include "MH_Media.h"

#include "MH_MediaPacketPoolInterface.h"

#include <queue>

class MHMEDIA_API MH_MediaPacketPool : public MH_MediaPacketPoolInterface
{
private:
	int m_size;
	bool m_expand;
	unsigned char* m_buffer;
	std::queue<MH_MediaPacket*> m_media_packets;

protected:
	virtual ~MH_MediaPacketPool();

	virtual MH_MediaPacket* OnGetMediaPacket(int size);
	virtual void OnReleasePacket(MH_MediaPacket* media_packet);
	virtual void OnClose();

public:
	MH_MediaPacketPool(const char* tag);

	bool Create(int size, int count, bool expand);
};

#endif
