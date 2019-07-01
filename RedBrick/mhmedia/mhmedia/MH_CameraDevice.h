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

#ifndef MH_CAMERA_DEVICE_H
#define MH_CAMERA_DEVICE_H

#include "MH_Media.h"
#include "MH_CaptureDevice.h"

#include <vector>

class MHMEDIA_API MH_CameraDevice
{
private:
	std::vector<bool> m_using_device;
	
protected:
	virtual bool OnInitialize();
	virtual void OnTerminate();
	
public:
	MH_CameraDevice();
	virtual ~MH_CameraDevice();

	bool Initialize();
	void Terminate();

	virtual void GetHardwareId(char* hardware_id) = 0;
	
	virtual int GetMaxWidth() = 0;
	virtual int GetMaxHeight() = 0;
	virtual int GetMaxFramerate() = 0;
	virtual int GetMaxFrameSize() = 0;
	
	virtual int GetCaptureNum() = 0;
	
	MH_CaptureDevice* ReserveCaptureDevice();
	void ReleaseCaptureDevice(MH_CaptureDevice* capture);
	
	virtual MH_CaptureDevice* GetCaptureDevice(int index) = 0;
	virtual int GetCaptureIndex(const char* device_id) = 0;
};

#endif
