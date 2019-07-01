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
#include "raspicamera/RaspiCameraDevice.h"

RaspiCameraDevice::RaspiCameraDevice()
	: m_capture_device(NULL)
{
}

RaspiCameraDevice::~RaspiCameraDevice()
{
}

bool RaspiCameraDevice::OnInitialize()
{
	if (m_capture_device != NULL)
	{
		return false;
	}
	
	m_capture_device = new RaspiCaptureDevice();
	if (!m_capture_device->Initialize("RaspiCaptureDevice"))
	{
		delete m_capture_device;
		m_capture_device = NULL;
		return false;
	}
	
	return true;
}

void RaspiCameraDevice::OnTerminate()
{
	if (m_capture_device == NULL)
	{
		return;
	}
	
	m_capture_device->Terminate();
	delete m_capture_device;
	m_capture_device = NULL;
}

void RaspiCameraDevice::GetHardwareId(char* hardware_id)
{
	strcpy(hardware_id, "RaspiCameraDevice");
}

int RaspiCameraDevice::GetMaxWidth()
{
	return 1920;
}

int RaspiCameraDevice::GetMaxHeight()
{
	return 1080;
}

int RaspiCameraDevice::GetMaxFramerate()
{
	return 60;
}

int RaspiCameraDevice::GetMaxFrameSize()
{
	return 128 * 1024;
}

int RaspiCameraDevice::GetCaptureNum()
{
	return 1;
}

MH_CaptureDevice* RaspiCameraDevice::GetCaptureDevice(int index)
{
	if (index != 0)
	{
		return NULL;
	}
	return m_capture_device;
}

int RaspiCameraDevice::GetCaptureIndex(const char* device_id)
{
	if (strcmp(m_capture_device->GetDeviceId(), device_id) == 0)
	{
		return 0;
	}
	
	return -1;
}
