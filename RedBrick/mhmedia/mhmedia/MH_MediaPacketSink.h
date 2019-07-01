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

#ifndef MH_MEDIA_PACKET_SINK_H
#define MH_MEDIA_PACKET_SINK_H

#include "MH_Media.h"

#include "MH_MediaPacket.h"

class MHMEDIA_API MH_MediaPacketSink
{
public:
	MH_MediaPacketSink();
	virtual ~MH_MediaPacketSink();
	
	virtual void Push(MH_MediaPacket* packet);
};

#endif
