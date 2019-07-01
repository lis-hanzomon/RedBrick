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

#ifndef MH_CAPTURE_DEVICE_H
#define MH_CAPTURE_DEVICE_H

#include "MH_Media.h"
#include "MH_MediaPacketSrc.h"

struct CaptureParam
{
	int width;
	int height;
	int framerate;
	int bitrate;
};

class MHMEDIA_API MH_CaptureDevice : public MH_MediaPacketSrc
{
protected:
	char m_device_id[80];
	CaptureParam m_param;

	virtual bool OnInitialize() = 0;
	virtual void OnTerminate() = 0;

	virtual bool OnStart(CaptureParam* param) = 0;
	virtual void OnStop() = 0;
	
	virtual bool OnSnapShot(MH_MediaPacket* packet) = 0;

public:
	MH_CaptureDevice();
	virtual ~MH_CaptureDevice();

	bool Initialize(const char* device_id);
	void Terminate();
	
	const char* GetDeviceId();

	bool Start(CaptureParam* param);
	void Stop();
	
	int GetWidth();
	int GetHeight();
	int GetBitrate();
	
	bool SnapShot(MH_MediaPacket* packet);
};

#endif
