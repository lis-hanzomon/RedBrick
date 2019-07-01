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

#ifndef RASPI_CAMERA_DEVICE_H
#define RASPI_CAMERA_DEVICE_H

#include "mhmedia/MH_CameraDevice.h"
#include "RaspiCaptureDevice.h"

class RaspiCameraDevice : public MH_CameraDevice
{
private:
	RaspiCaptureDevice* m_capture_device;

protected:
	virtual bool OnInitialize();
	virtual void OnTerminate();
	
public:
	RaspiCameraDevice();
	virtual ~RaspiCameraDevice();

	virtual void GetHardwareId(char* hardware_id);
	virtual int GetMaxWidth();
	virtual int GetMaxHeight();
	virtual int GetMaxFramerate();
	virtual int GetMaxFrameSize();
	
	virtual int GetCaptureNum();
	virtual MH_CaptureDevice* GetCaptureDevice(int index);
	virtual int GetCaptureIndex(const char* device_id);
};

#endif
