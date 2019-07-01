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
#include "mhmedia/MH_CameraDevice.h"

MH_CameraDevice::MH_CameraDevice()
{
}

MH_CameraDevice::~MH_CameraDevice()
{
}

bool MH_CameraDevice::Initialize()
{
	if (!OnInitialize())
	{
		return false;
	}
	
	m_using_device.resize(GetCaptureNum(), false);
	
	return true;
}

bool MH_CameraDevice::OnInitialize()
{
	return true;
}

void MH_CameraDevice::Terminate()
{
	OnTerminate();
}

void MH_CameraDevice::OnTerminate()
{
}

MH_CaptureDevice* MH_CameraDevice::ReserveCaptureDevice()
{
	MH_CaptureDevice* capture = NULL;
	
	int capture_num = GetCaptureNum();
	for(int i = 0; i < capture_num; i++)
	{
		if (!m_using_device[i])
		{
			capture = GetCaptureDevice(i);
			m_using_device[i] = true;
			break;
		}
	}
	
	return capture;
}

void MH_CameraDevice::ReleaseCaptureDevice(MH_CaptureDevice* capture)
{
	int capture_num = GetCaptureNum();
	for(int i = 0; i < capture_num; i++)
	{
		if (GetCaptureDevice(i) == capture)
		{
			m_using_device[i] = false;
			break;
		}
	}
}
