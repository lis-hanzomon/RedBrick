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
#include <uuid/uuid.h>
#include "RaspiOnvifPlatform.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

RaspiOnvifPlatform::RaspiOnvifPlatform()
	: m_ptz_device(NULL)
{
	DBG_TRACE("Enter RaspiOnvifPlatform::RaspiOnvifPlatform");
	
	DBG_TRACE("Exit RaspiOnvifPlatform::RaspiOnvifPlatform");
}

RaspiOnvifPlatform::~RaspiOnvifPlatform()
{
	DBG_TRACE("Enter RaspiOnvifPlatform::~RaspiOnvifPlatform");
	
	DBG_TRACE("Exit RaspiOnvifPlatform::~RaspiOnvifPlatform");
}

bool RaspiOnvifPlatform::Initialize(const char* path)
{
	DBG_TRACE("Enter RaspiOnvifPlatform::Initialize");
	DBG_PRINT("path = %s", path);
	
	strcpy(m_path, path);
	
	LoadDeviceInfo();
	
	DefaultSetting();
	if (!LoadSetting())
	{
		SaveSetting();
	}
	
	if (!m_network_setting.Initialize(m_if_name))
	{
		return false;
	}
	
	if (!m_user_setting.Initialize(m_path))
	{
		return false;
	}
	
	if (!m_profile_setting.Initialize(m_path, m_is_supported_ptz))
	{
		return false;
	}
	
	m_camera_device = new RaspiCameraDevice();
	if (!m_camera_device->Initialize())
	{
		return false;
	}
	
	if (m_is_supported_ptz)
	{
		m_ptz_device = new RaspiPtzDevice();
		if (!m_ptz_device->Initialize())
		{
			return false;
		}
	}
	
	DBG_TRACE("Exit RaspiOnvifPlatform::Initialize");
	
	return true;
}

void RaspiOnvifPlatform::Terminate()
{
	DBG_TRACE("Enter RaspiOnvifPlatform::Terminate");
	
	if (m_camera_device != NULL)
	{
		m_camera_device->Terminate();
		delete m_camera_device;
		m_camera_device = NULL;
	}
	
	if (m_ptz_device != NULL)
	{
		m_ptz_device->Terminate();
		delete m_ptz_device;
		m_ptz_device = NULL;
	}
	
	DBG_TRACE("Exit RaspiOnvifPlatform::Terminate");
}

bool RaspiOnvifPlatform::LoadDeviceInfo()
{
	DBG_TRACE("Enter RaspiOnvifPlatform::LoadDeviceInfo");
	
	memset(&m_device_info, 0x00, sizeof(m_device_info));
	m_is_supported_ptz = false;
	memset(m_endpoint_reference, 0x00, sizeof(m_endpoint_reference));
	
	char filepath[255];
	sprintf(filepath, "%s/device_info.json", m_path);
	struct json_object* jo = json_object_from_file(filepath);
	if (jo == NULL)
	{
		return false;
	}
	
	json_object_object_foreach(jo, key, val)
	{
		if(json_object_is_type(val, json_type_string))
		{
			if (strcmp(key, "Manufacturer") == 0)
			{
				strcpy(m_device_info.Manufacturer, json_object_get_string(val));
			}
			else if (strcmp(key, "Model") == 0)
			{
				strcpy(m_device_info.Model, json_object_get_string(val));
			}
			else if (strcmp(key, "FirmwareVersion") == 0)
			{
				strcpy(m_device_info.FirmwareVersion, json_object_get_string(val));
			}
			else if (strcmp(key, "SerialNumber") == 0)
			{
				strcpy(m_device_info.SerialNumber, json_object_get_string(val));
			}
			else if (strcmp(key, "HardwareId") == 0)
			{
				strcpy(m_device_info.HardwareId, json_object_get_string(val));
			}
			else if (strcmp(key, "EndpointReference") == 0)
			{
				strcpy(m_endpoint_reference, json_object_get_string(val));
			}
		}
		else if(json_object_is_type(val, json_type_int))
		{
			if (strcmp(key, "PtzEnable") == 0)
			{
				if (json_object_get_int(val) == 0)
				{
					m_is_supported_ptz = false;
				}
				else
				{
					m_is_supported_ptz = true;
				}
			}
		}
	}
	json_object_put(jo);
	
	if (strlen(m_endpoint_reference) == 0)
	{
		uuid_t uuid;
		uuid_generate(uuid);
		uuid_unparse(uuid, m_endpoint_reference);
		
		SaveDeviceInfo();
	}
	
	DBG_PRINT("Manufacturer       = %s", m_device_info.Manufacturer);
	DBG_PRINT("Model              = %s", m_device_info.Model);
	DBG_PRINT("m_is_supported_ptz = %d", m_is_supported_ptz);
	DBG_PRINT("EndpointReference  = %s", m_endpoint_reference);
	
	DBG_TRACE("Exit RaspiOnvifPlatform::LoadDeviceInfo");
	
	return true;
}

bool RaspiOnvifPlatform::SaveDeviceInfo()
{
	DBG_TRACE("Enter RaspiOnvifPlatform::SaveDeviceInfo");

	char filepath[255];
	sprintf(filepath, "%s/device_info.json", m_path);
	FILE* fp = fopen(filepath, "w");
	if (fp == NULL)
	{
		DBG_PRINT("Error: fopen = NULL");
		return false;
	}
	
	fprintf(
		fp, 
		"{\n"
		"\t\"Manufacturer\" : \"%s\",\n"
		"\t\"Model\" : \"%s\",\n"
		"\t\"FirmwareVersion\" : \"%s\",\n"
		"\t\"SerialNumber\" : \"%s\",\n"
		"\t\"HardwareId\" : \"%s\",\n"
		"\t\"PtzEnable\" : %d,\n"
		"\t\"EndpointReference\" : \"%s\"\n"
        "}\n",
		m_device_info.Manufacturer,
		m_device_info.Model,
		m_device_info.FirmwareVersion,
		m_device_info.SerialNumber,
		m_device_info.HardwareId,
		m_is_supported_ptz ? 1 : 0,
		m_endpoint_reference
	);
	
	fclose(fp);
	
	DBG_TRACE("Exit RaspiOnvifPlatform::SaveDeviceInfo");
	
	return true;
}

void RaspiOnvifPlatform::DefaultSetting()
{
	DBG_TRACE("Enter RaspiOnvifPlatform::DefaultSetting");
	
	strcpy(m_identification_name,     "raspicam");
	strcpy(m_identification_location, "country/Japan");
	
	m_discovery_enable = true;
	
	m_onvif_port_no     = 9000;
	m_snap_shot_port_no = 8080;
	m_rtsp_port_no      = 8554;
	
	m_rtsp_buffering_sec = 5;
	
	strcpy(m_recording_path, "/media/rec");
	m_recording_interval = 300;
	m_recording_file_num = -1;
	m_recording_keep_size = 4000;
	m_recording_mode = false;
	
	strcpy(m_snap_shot_filepath, "/var/tmp");
	m_snap_shot_width = 640;
	
	DBG_TRACE("Exit RaspiOnvifPlatform::DefaultSetting");
}

bool RaspiOnvifPlatform::LoadSetting()
{
	DBG_TRACE("Enter RaspiOnvifPlatform::LoadSetting");
	
	char filepath[255];
	sprintf(filepath, "%s/onvif_setting.json", m_path);
	struct json_object* jo = json_object_from_file(filepath);
	if (jo == NULL)
	{
		return false;
	}
	
	json_object_object_foreach(jo, key, val)
	{
		if(json_object_is_type(val, json_type_string))
		{
			if (strcmp(key, "IdentificationName") == 0)
			{
				strcpy(m_identification_name, json_object_get_string(val));
			}
			else if (strcmp(key, "IdentificationLocation") == 0)
			{
				strcpy(m_identification_location, json_object_get_string(val));
			}
			else if (strcmp(key, "Interface") == 0)
			{
				strcpy(m_if_name, json_object_get_string(val));
			}
			else if (strcmp(key, "SnapShotPath") == 0)
			{
				strcpy(m_snap_shot_filepath, json_object_get_string(val));
			}
			else if (strcmp(key, "RecordingPath") == 0)
			{
				strcpy(m_recording_path, json_object_get_string(val));
			}
		}
		else if(json_object_is_type(val, json_type_int))
		{
			if (strcmp(key, "DiscoveryMode") == 0)
			{
				if (json_object_get_int(val) == 0)
				{
					m_discovery_enable = false;
				}
				else
				{
					m_discovery_enable = true;
				}
			}
			else if (strcmp(key, "OnvifPortNo") == 0)
			{
				m_onvif_port_no = json_object_get_int(val);
			}
			else if (strcmp(key, "RtspPortNo") == 0)
			{
				m_rtsp_port_no = json_object_get_int(val);
			}
			else if (strcmp(key, "SnapShotPortNo") == 0)
			{
				m_snap_shot_port_no = json_object_get_int(val);
			}
			else if (strcmp(key, "RecordingInterval") == 0)
			{
				m_recording_interval = json_object_get_int(val);
			}
			else if (strcmp(key, "RecordingFileNum") == 0)
			{
				m_recording_file_num = json_object_get_int(val);
			}
			else if (strcmp(key, "RecordingKeepSize") == 0)
			{
				m_recording_keep_size = json_object_get_int(val);
			}
			else if (strcmp(key, "RecordingMode") == 0)
			{
				if (json_object_get_int(val) == 0)
				{
					m_recording_mode = false;
				}
				else
				{
					m_recording_mode = true;
				}
			}
			if (strcmp(key, "SnapShotWidth") == 0)
			{
				m_snap_shot_width = json_object_get_int(val);
			}
		}
	}
	json_object_put(jo);
	
	DBG_TRACE("Exit RaspiOnvifPlatform::LoadSetting");
	
	return true;
}

bool RaspiOnvifPlatform::SaveSetting()
{
	DBG_TRACE("Enter RaspiOnvifPlatform::SaveSetting");

	char filepath[255];
	sprintf(filepath, "%s/onvif_setting.json", m_path);
	FILE* fp = fopen(filepath, "w");
	if (fp == NULL)
	{
		DBG_PRINT("Error: fopen = NULL");
		return false;
	}
	
	fprintf(
		fp, 
		"{\n"
		"\t\"IdentificationName\" : \"%s\",\n"
		"\t\"IdentificationLocation\" : \"%s\",\n"
		"\t\"DiscoveryMode\" : %d,\n"
		"\t\"Interface\" : \"%s\",\n"
		"\t\"OnvifPortNo\" : %d,\n"
		"\t\"RtspPortNo\" : %d,\n"
		"\t\"SnapShotPortNo\" : %d,\n"
		"\t\"RecordingPath\" : \"%s\",\n"
		"\t\"RecordingInterval\" : %d,\n"
		"\t\"RecordingFileNum\" : %d,\n"
		"\t\"RecordingKeepSize\" : %d,\n"
		"\t\"RecordingMode\" : %d,\n"
		"\t\"SnapShotPath\" : \"%s\",\n"
		"\t\"SnapShotWidth\" : %d\n"
        "}\n",
        m_identification_name,
        m_identification_location,
		m_discovery_enable ? 1 : 0,
		m_if_name,
		m_onvif_port_no,
		m_rtsp_port_no,
		m_snap_shot_port_no,
		m_recording_path,
		m_recording_interval,
		m_recording_file_num,
		m_recording_keep_size,
		m_recording_mode ? 1 : 0,
		m_snap_shot_filepath,
		m_snap_shot_width
	);
	
	fclose(fp);
	
	DBG_TRACE("Exit RaspiOnvifPlatform::SaveSetting");
	
	return true;
}

bool RaspiOnvifPlatform::OnUpdateSetting()
{
	return SaveSetting();
}

MH_CameraDevice* RaspiOnvifPlatform::GetCameraDevice()
{
	return m_camera_device;
}

SZ_PtzDevice* RaspiOnvifPlatform::GetPtzDevice(const char* ptz_token)
{
	if (strlen(ptz_token) == 0)
	{
		return NULL;
	}
	return m_ptz_device;
}

SZ_NetworkSetting* RaspiOnvifPlatform::GetNetworkSetting()
{
	return &m_network_setting;
}

SZ_UserSetting* RaspiOnvifPlatform::GetUserSetting()
{
	return &m_user_setting;
}

SZ_ProfileSetting* RaspiOnvifPlatform::GetProfileSetting()
{
	return &m_profile_setting;
}

bool RaspiOnvifPlatform::SystemReboot()
{
	DBG_TRACE("Enter RaspiOnvifPlatform::SystemReboot");
	
	DBG_TRACE("Exit RaspiOnvifPlatform::SystemReboot");
	
	return true;
}

bool RaspiOnvifPlatform::FactoryReset(int type)
{
	DBG_TRACE("Enter RaspiOnvifPlatform::FactoryReset");
	
	DefaultSetting();
	
	SaveSetting();
	
	DBG_TRACE("Exit RaspiOnvifPlatform::FactoryReset");
	
	return true;
}
