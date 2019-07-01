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
#include "mhmedia/MH_CaptureDevice.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_CaptureDevice::MH_CaptureDevice()
{
	strcpy(m_device_id, "");
	
	m_param.width     = 0;
	m_param.height    = 0;
	m_param.framerate = 0;
	m_param.bitrate   = 0;
}

MH_CaptureDevice::~MH_CaptureDevice()
{
}

bool MH_CaptureDevice::Initialize(const char* device_id)
{
	strcpy(m_device_id, device_id);
	
	m_param.width     = 0;
	m_param.height    = 0;
	m_param.framerate = 0;
	m_param.bitrate   = 0;
	
	if (!OnInitialize())
	{
		Terminate();
		return false;
	}
	
	return true;
}

void MH_CaptureDevice::Terminate()
{
	OnTerminate();
}

const char* MH_CaptureDevice::GetDeviceId()
{
	return m_device_id;
}

bool MH_CaptureDevice::Start(CaptureParam* param)
{
	m_param = *param;
	
	if (!OnStart(param))
	{
		Stop();
		return false;
	}
	
	return true;
}

void MH_CaptureDevice::Stop()
{
	OnStop();
}

int MH_CaptureDevice::GetWidth()
{
	return m_param.width;
}

int MH_CaptureDevice::GetHeight()
{
	return m_param.height;
}

int MH_CaptureDevice::GetBitrate()
{
	return m_param.bitrate;
}

bool MH_CaptureDevice::SnapShot(MH_MediaPacket* packet)
{
	return OnSnapShot(packet);
}
