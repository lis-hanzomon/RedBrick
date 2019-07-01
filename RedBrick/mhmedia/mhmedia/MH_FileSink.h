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

#ifndef MH_FILE_SINK_H
#define MH_FILE_SINK_H

#include "MH_Media.h"

#include "MH_MediaPacketSink.h"

class MHMEDIA_API MH_FileSink : public MH_MediaPacketSink
{
private:
	FILE* m_fp;

public:
	MH_FileSink();
	virtual ~MH_FileSink();
	
	bool Open(const char* filename);
	void Close();
	
	virtual void Push(MH_MediaPacket* packet);
};

#endif
