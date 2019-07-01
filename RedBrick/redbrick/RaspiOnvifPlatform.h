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

#ifndef RASPI_ONVIF_PLATFORM_H
#define RASPI_ONVIF_PLATFORM_H

#include "mhonvif/SZ_OnvifPlatform.h"
#include "SZ_UserSettingImpl.h"
#include "SZ_NetworkSettingImpl.h"
#include "SZ_ProfileSettingImpl.h"
#include "raspicamera/RaspiCameraDevice.h"
#include "RaspiPtzDevice.h"

class RaspiOnvifPlatform : public SZ_OnvifPlatform
{
private:
	char m_path[255];

	RaspiCameraDevice* m_camera_device;
	RaspiPtzDevice* m_ptz_device;
	SZ_NetworkSettingImpl m_network_setting;
	SZ_UserSettingImpl m_user_setting;
	SZ_ProfileSettingImpl m_profile_setting;
	
	bool LoadDeviceInfo();
	bool SaveDeviceInfo();
	
	void DefaultSetting();
	bool LoadSetting();
	bool SaveSetting();

protected:
	virtual bool OnUpdateSetting();

public:
	RaspiOnvifPlatform();
	virtual ~RaspiOnvifPlatform();
	
	bool Initialize(const char* path);
	void Terminate();
	
	virtual MH_CameraDevice* GetCameraDevice();
	virtual SZ_PtzDevice* GetPtzDevice(const char* ptz_token);
	
	virtual SZ_NetworkSetting* GetNetworkSetting();
	virtual SZ_UserSetting* GetUserSetting();
	virtual SZ_ProfileSetting* GetProfileSetting();
	
	virtual bool SystemReboot();
	virtual bool FactoryReset(int type);
};

#endif
