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
#include "SZ_ProfileSettingImpl.h"

#include <json-c/json.h>

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

SZ_ProfileSettingImpl::SZ_ProfileSettingImpl()
{
	DBG_TRACE("Enter SZ_ProfileSettingImpl::SZ_ProfileSettingImpl");
	
	DBG_TRACE("Exit SZ_ProfileSettingImpl::SZ_ProfileSettingImpl");
}

SZ_ProfileSettingImpl::~SZ_ProfileSettingImpl()
{
}

bool SZ_ProfileSettingImpl::Initialize(const char* path, bool is_ptz_support)
{
	DBG_TRACE("Enter SZ_ProfileSettingImpl::Initialize");
	
	strcpy(m_path, path);
	
	LoadVideoSourceConfig();
	LoadVideoEncoderConfig();
	
	if (is_ptz_support)
	{
		LoadPtzConfig();
	}
	
	LoadProfile();
	
	DBG_TRACE("Exit SZ_ProfileSettingImpl::Initialize");
	
	return true;
}

void SZ_ProfileSettingImpl::LoadVideoSourceConfig()
{
	DBG_TRACE("Enter LoadVideoSourceConfig");
	
 	ONVIF_VIDEO_SOURCE_CONFIG_INFO info;
 	memset(&info, 0x00, sizeof(info));

	strcpy(info.name,         "Config1");
	strcpy(info.token,        "VideoSourceConfig1");
	strcpy(info.source_token, "RaspiCameraDevice");
	info.use_count = 0;
	info.x = 0;
	info.y = 0;
	info.width  = 1920;
	info.height = 1080;
	
	m_source_config_info[info.token] = info;
	
	DBG_TRACE("Exit LoadVideoSourceConfig");
}

void SZ_ProfileSettingImpl::LoadVideoEncoderConfig()
{
	DBG_TRACE("Enter LoadVideoEncoderConfig");
	
	char filepath[255];
	sprintf(filepath, "%s/video_encoder_config.json", m_path);
	struct json_object* jo = json_object_from_file(filepath);
	if (jo != NULL)
	{
		DBG_PRINT("jo != NULL");
		json_object_object_foreach(jo, key, val)
		{
			DBG_PRINT("key = %s", key);
			if(json_object_is_type(val, json_type_array))
			{
				if (strcmp(key, "VideoEncoderConfig") == 0)
				{
					int len = json_object_array_length(val);
					for (int i = 0; i < len; i++)
					{
					 	ONVIF_VIDEO_ENCODER_CONFIG_INFO info;
					 	memset(&info, 0x00, sizeof(info));
						
						struct json_object * configs = json_object_array_get_idx(val, i);
						json_object_object_foreach(configs, config_key, config_val)
						{
							if(json_object_is_type(config_val, json_type_string))
							{
								if (strcmp(config_key, "Name") == 0)
								{
									strcpy(info.name, json_object_get_string(config_val));
								}
								else if (strcmp(config_key, "Token") == 0)
								{
									strcpy(info.token, json_object_get_string(config_val));
								}
							}
							else if(json_object_is_type(config_val, json_type_int))
							{
								if (strcmp(config_key, "Quality") == 0)
								{
									info.quality = json_object_get_int(config_val);
									
									GetSupportedResolution(info.quality, &info.width, &info.height);
									
									switch(info.quality)
									{
									case 0:
										info.bitrate = 1000000;
										break;
									
									case 1:
										info.bitrate = 2000000;
										break;
									}
								}
	   					 	}
	   					 }

	   					 info.use_count = 0;
	   					 
	   					 DBG_PRINT("Name        = %s", info.name);
	   					 DBG_PRINT("Token       = %s", info.token);
	   					 DBG_PRINT("Quality     = %d", info.quality);
	   					 
	   					 m_encoder_config_info[info.token] = info;
					 }
				}
			}
		}
	}
	
	DBG_TRACE("Exit LoadVideoEncoderConfig");
}

void SZ_ProfileSettingImpl::LoadPtzConfig()
{
 	ONVIF_PTZ_CONFIG_INFO info;
 	memset(&info, 0x00, sizeof(info));
	
	strcpy(info.name,       "Config1");
	strcpy(info.token,      "PtzConfig1");
	strcpy(info.node_token, "PtzNodeToken1");

	info.use_count = 0;
	
	info.default_speed_x = 0.5;
	info.default_speed_y = 0.5;
	
	m_ptz_config_info[info.token] = info;
}

void SZ_ProfileSettingImpl::LoadProfile()
{
	DBG_TRACE("Enter LoadProfile");
	
	char filepath[255];
	sprintf(filepath, "%s/profile.json", m_path);
	struct json_object* jo = json_object_from_file(filepath);
	if (jo != NULL)
	{
		json_object_object_foreach(jo, key, val)
		{
			if(json_object_is_type(val, json_type_array))
			{
				if (strcmp(key, "Profiles") == 0)
				{
					int len = json_object_array_length(val);
					for (int i = 0; i < len; i++)
					{
					 	ONVIF_PROFILE_INFO info;
					 	memset(&info, 0x00, sizeof(info));
						
						struct json_object * configs = json_object_array_get_idx(val, i);
						json_object_object_foreach(configs, config_key, config_val)
						{
							if(json_object_is_type(config_val, json_type_string))
							{
								if (strcmp(config_key, "Name") == 0)
								{
									strcpy(info.name, json_object_get_string(config_val));
								}
								else if (strcmp(config_key, "Token") == 0)
								{
									strcpy(info.token, json_object_get_string(config_val));
								}
								else if (strcmp(config_key, "VideoSourceConfigToken") == 0)
								{
									strcpy(info.video_source_config_token, json_object_get_string(config_val));
									
									std::map<std::string, ONVIF_VIDEO_SOURCE_CONFIG_INFO>::iterator it = m_source_config_info.find(
										info.video_source_config_token
									);
									if (it == m_source_config_info.end())
									{
										strcpy(info.video_source_config_token, "");
									}
									else
									{
										(it->second).use_count ++;
									}
								}
								else if (strcmp(config_key, "VideoEncoderToken") == 0)
								{
									strcpy(info.video_encoder_token, json_object_get_string(config_val));
									
									std::map<std::string, ONVIF_VIDEO_ENCODER_CONFIG_INFO>::iterator it = m_encoder_config_info.find(
										info.video_encoder_token
									);
									if (it == m_encoder_config_info.end())
									{
										strcpy(info.video_encoder_token, "");
									}
									else
									{
										(it->second).use_count ++;
									}
								}
								else if (strcmp(config_key, "PtzToken") == 0)
								{
									strcpy(info.ptz_token, json_object_get_string(config_val));
									
									std::map<std::string, ONVIF_PTZ_CONFIG_INFO>::iterator it = m_ptz_config_info.find(
										info.ptz_token
									);
									if (it == m_ptz_config_info.end())
									{
										strcpy(info.ptz_token, "");
									}
									else
									{
										(it->second).use_count ++;
									}
								}
							}
						}
						
						if (m_profile_info.size() == 0)
						{
							// ç≈èâÇÃÇPÇ¬ñ⁄ÇÕçÌèúïsâ¬Ç∆Ç∑ÇÈ
							info.fixed = 1;
						}
						
	   					DBG_PRINT("Name                   = %s", info.name);
	   					DBG_PRINT("Token                  = %s", info.token);
	   					DBG_PRINT("VideoSourceConfigToken = %s", info.video_source_config_token);
	   					DBG_PRINT("VideoEncoderToken      = %s", info.video_encoder_token);
	   					DBG_PRINT("PtzToken               = %s", info.ptz_token);
	   					DBG_PRINT("Fixed                  = %d", info.fixed);
	   					 
	   					m_profile_info.push_back(info);
					 }
				}
			}
		}
	}
	
	DBG_TRACE("Exit LoadProfile");
}

bool SZ_ProfileSettingImpl::SaveProfile()
{
	DBG_TRACE("Enter SaveProfile");

	char filepath[255];
	sprintf(filepath, "%s/profile.json", m_path);
	FILE* fp = fopen(filepath, "w");
	if (fp == NULL)
	{
		DBG_PRINT("Error: fopen = NULL");
		return false;
	}
	
	fprintf(
		fp, 
		"{\n"
		"\t\"Profiles\" : [\n"
		);
	
	for(int i = 0; i < m_profile_info.size(); i++)
	{
		fprintf(
			fp,
			"\t\t{\n"
			"\t\t\t\"Name\" : \"%s\",\n"
			"\t\t\t\"Token\" : \"%s\",\n"
			"\t\t\t\"VideoSourceConfigToken\" : \"%s\",\n"
			"\t\t\t\"VideoEncoderToken\" : \"%s\",\n"
			"\t\t\t\"PtzToken\" : \"%s\"\n",
			m_profile_info[i].name,
			m_profile_info[i].token,
			m_profile_info[i].video_source_config_token,
			m_profile_info[i].video_encoder_token,
			m_profile_info[i].ptz_token
			);
		
		if (i + 1 < m_profile_info.size())
		{
			fprintf(fp, "\t\t},\n");
		}
		else
		{
			fprintf(fp, "\t\t}\n");
		}
	}
	
	fprintf(
		fp, 
		"\t]\n"
		"}\n"
		);
	
	fclose(fp);
	
	DBG_TRACE("Exit SaveProfile");
	
	return true;
}

bool SZ_ProfileSettingImpl::OnProfileUpdate(const char* token)
{
	return SaveProfile();
}

int SZ_ProfileSettingImpl::GetSupportedResolutionCount()
{
	return 2;
}

bool SZ_ProfileSettingImpl::GetSupportedResolution(int index, int* width, int* height)
{
	switch(index)
	{
	case 0:
		*width  = 640;
		*height = 360;
		break;
	
	case 1:
		*width  = 1280;
		*height = 720;
		break;
		
	default:
		return false;
	}
	
	return true;
}

bool SZ_ProfileSettingImpl::OnVideoEncoderUpdate(const char* token, ONVIF_VIDEO_ENCODER_CONFIG_INFO* encoder_config_info)
{
	DBG_TRACE("Enter SZ_ProfileSettingImpl::OnVideoEncoderUpdate");
	DBG_PRINT("quality = %d", encoder_config_info->quality);
	
	if (!GetSupportedResolution(
		encoder_config_info->quality, 
		&encoder_config_info->width,
		&encoder_config_info->height
		))
	{
		return false;
	}
	
	switch(encoder_config_info->quality)
	{
	case 0:
		encoder_config_info->bitrate = 1000000;
		break;
	
	case 1:
		encoder_config_info->bitrate = 2000000;
		break;
	}
	
	char filepath[255];
	sprintf(filepath, "%s/video_encoder_config.json", m_path);
	FILE* fp = fopen(filepath, "w");
	if (fp == NULL)
	{
		DBG_PRINT("Error: fopen = NULL");
		return false;
	}
	
	fprintf(
		fp, 
		"{\n"
		"\t\"VideoEncoderConfig\" : [\n"
		);
	
	int i = 0;
	for(std::map<std::string, ONVIF_VIDEO_ENCODER_CONFIG_INFO>::iterator it = m_encoder_config_info.begin(); it != m_encoder_config_info.end(); it++)
	{
		fprintf(
			fp,
			"\t\t{\n"
			"\t\t\t\"Name\" : \"%s\",\n"
			"\t\t\t\"Token\" : \"%s\",\n"
			"\t\t\t\"Quality\" : %d\n",
			(it->second).name,
			(it->second).token,
			(it->second).quality
			);
		
		if (i + 1 < m_encoder_config_info.size())
		{
			fprintf(fp, "\t\t},\n");
		}
		else
		{
			fprintf(fp, "\t\t}\n");
		}
		i++;
	}
	
	fprintf(
		fp, 
		"\t]\n"
		"}\n"
		);
	
	fclose(fp);
	
	DBG_TRACE("Exit SZ_ProfileSettingImpl::OnVideoEncoderUpdate");

	return true;
}
