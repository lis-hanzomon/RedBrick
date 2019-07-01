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
#include <json-c/json.h>
#include "mhonvif/SZ_OnvifPlatform.h"

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

SZ_OnvifPlatform::SZ_OnvifPlatform()
{
}

SZ_OnvifPlatform::~SZ_OnvifPlatform()
{
}

void SZ_OnvifPlatform::GetDeviceInfo(ONVIF_DEVICE_INFO* device_info)
{
	memcpy(device_info, &m_device_info, sizeof(ONVIF_DEVICE_INFO));
}

void SZ_OnvifPlatform::GetOnvifId(char* name, char* location)
{
	strcpy(name,     m_identification_name);
	strcpy(location, m_identification_location);
}

bool SZ_OnvifPlatform::SetOnvifId(const char* name, const char* location)
{
	strcpy(m_identification_name,     name);
	strcpy(m_identification_location, location);
	
	return OnUpdateSetting();
}

bool SZ_OnvifPlatform::GetDiscoveryEnable()
{
	return m_discovery_enable;
}

bool SZ_OnvifPlatform::SetDiscoveryEnable(bool discovery_enable)
{
	m_discovery_enable = discovery_enable;

	return OnUpdateSetting();
}

int SZ_OnvifPlatform::GetOnvifPortNo()
{
	return m_onvif_port_no;
}

int SZ_OnvifPlatform::GetSnapShotPortNo()
{
	return m_snap_shot_port_no;
}

bool SZ_OnvifPlatform::SetSnapShotPortNo(int port_no)
{
	m_snap_shot_port_no = port_no;
	
	return OnUpdateSetting();
}

int SZ_OnvifPlatform::GetRtspPortNo()
{
	return m_rtsp_port_no;
}

bool SZ_OnvifPlatform::SetRtspPortNo(int port_no)
{
	m_rtsp_port_no = port_no;
	
	return OnUpdateSetting();
}

int SZ_OnvifPlatform::GetRtspBufferingSec()
{
	return m_rtsp_buffering_sec;
}

int SZ_OnvifPlatform::GetSnapShotWidth()
{
	return m_snap_shot_width;
}

const char* SZ_OnvifPlatform::GetSnapShotFilePath()
{
	return m_snap_shot_filepath;
}

bool SZ_OnvifPlatform::GetRecordingMode()
{
	return m_recording_mode;
}

void SZ_OnvifPlatform::SetRecordingMode(bool mode)
{
	m_recording_mode = mode;
}

const char* SZ_OnvifPlatform::GetRecordingPath()
{
	return m_recording_path;
}

int SZ_OnvifPlatform::GetRecordingInterval()
{
	return m_recording_interval;
}

int SZ_OnvifPlatform::GetRecordingFileNum()
{
	return m_recording_file_num;
}

int SZ_OnvifPlatform::GetRecordingKeepSize()
{
	return m_recording_keep_size;
}

bool SZ_OnvifPlatform::FactoryReset(int type)
{
	return true;
}

bool SZ_OnvifPlatform::SystemReboot()
{
	return true;
}

bool SZ_OnvifPlatform::IsSupportedPTZ()
{
	return m_is_supported_ptz;
}

const char* SZ_OnvifPlatform::GetEndpointReference()
{
	return m_endpoint_reference;
}

SZ_PtzDevice* SZ_OnvifPlatform::GetPtzDevice(const char* ptz_token)
{
	return NULL;
}
