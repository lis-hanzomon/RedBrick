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

#ifndef SZ_ONVIF_PLATFORM_H
#define SZ_ONVIF_PLATFORM_H

#include "SZ17.h"

#include "mhmedia/MH_CameraDevice.h"
#include "SZ_OnvifInfo.h"
#include "SZ_PtzDevice.h"

#include "SZ_UserSetting.h"
#include "SZ_NetworkSetting.h"
#include "SZ_ProfileSetting.h"

class SZ17_API SZ_OnvifPlatform
{
protected:
	ONVIF_DEVICE_INFO m_device_info;

	char m_identification_name[255];
	char m_identification_location[255];

	bool m_discovery_enable;
	
	char m_if_name[20];
	
	unsigned short m_onvif_port_no;
	unsigned short m_snap_shot_port_no;
	unsigned short m_rtsp_port_no;
	
	int m_rtsp_buffering_sec;

	int m_snap_shot_width;
	char m_snap_shot_filepath[255];
	
	bool m_recording_mode;
	char m_recording_path[255];
	int m_recording_interval;
	int m_recording_file_num;
	int m_recording_keep_size;

	bool m_is_supported_ptz;

	char m_endpoint_reference[40];
	
	virtual bool OnUpdateSetting() = 0;

public:
	SZ_OnvifPlatform();
	virtual ~SZ_OnvifPlatform();
	
	void GetDeviceInfo(ONVIF_DEVICE_INFO* device_info);

	void GetOnvifId(char* name, char* location);
	bool SetOnvifId(const char* name, const char* location);

	bool GetDiscoveryEnable();
	bool SetDiscoveryEnable(bool discovery_enable);

	int GetOnvifPortNo();
	
	int GetSnapShotPortNo();
	bool SetSnapShotPortNo(int port_no);
	
	int GetRtspPortNo();
	bool SetRtspPortNo(int port_no);
	
	int GetRtspBufferingSec();
	
	int GetSnapShotWidth();
	const char* GetSnapShotFilePath();
	
	bool GetRecordingMode();
	void SetRecordingMode(bool mode);
	
	const char* GetRecordingPath();
	int GetRecordingInterval();
	int GetRecordingFileNum();
	int GetRecordingKeepSize();
	
	bool IsSupportedPTZ();
	
	const char* GetEndpointReference();
	
	virtual MH_CameraDevice* GetCameraDevice() = 0;
	virtual SZ_PtzDevice* GetPtzDevice(const char* ptz_token);

	virtual SZ_UserSetting* GetUserSetting() = 0;
	virtual SZ_NetworkSetting* GetNetworkSetting() = 0;
	virtual SZ_ProfileSetting* GetProfileSetting() = 0;
	
	virtual bool FactoryReset(int type);
	virtual bool SystemReboot();
};

#endif
