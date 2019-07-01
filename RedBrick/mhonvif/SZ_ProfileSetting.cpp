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
#include "mhonvif/SZ_ProfileSetting.h"

#include <uuid/uuid.h>

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

SZ_ProfileSetting::SZ_ProfileSetting()
{
	DBG_TRACE("Enter SZ_ProfileSetting::SZ_ProfileSetting");
	
	DBG_TRACE("Exit SZ_ProfileSetting::SZ_ProfileSetting");
}

SZ_ProfileSetting::~SZ_ProfileSetting()
{
	DBG_TRACE("Enter SZ_ProfileSetting::~SZ_ProfileSetting");
	
	DBG_TRACE("Exit SZ_ProfileSetting::~SZ_ProfileSetting");
}

int SZ_ProfileSetting::GetVideoSourceConfigCount()
{
	return m_source_config_info.size();
}

int SZ_ProfileSetting::GetCompatibleVideoSourceConfigCount(int width, int height)
{
	return m_source_config_info.size();
}

bool SZ_ProfileSetting::GetVideoSourceConfig(int index, ONVIF_VIDEO_SOURCE_CONFIG_INFO* video_source_config_info)
{
	if (m_source_config_info.size() <= index)
	{
		return false;
	}
	
	std::map<std::string, ONVIF_VIDEO_SOURCE_CONFIG_INFO>::iterator it = m_source_config_info.begin();
	for(int i = 0; i < index; i++)
	{
		it ++;
	}
	memcpy(video_source_config_info, &(it->second), sizeof(ONVIF_VIDEO_SOURCE_CONFIG_INFO));
	
	return true;
}

bool SZ_ProfileSetting::FindVideoSourceConfig(const char* token, ONVIF_VIDEO_SOURCE_CONFIG_INFO* video_source_config_info)
{
	std::map<std::string, ONVIF_VIDEO_SOURCE_CONFIG_INFO>::iterator it = m_source_config_info.find(token);
	if (it == m_source_config_info.end())
	{
		DBG_PRINT("Error: NotFound");
		return false;
	}
	memcpy(video_source_config_info, &(it->second), sizeof(ONVIF_VIDEO_SOURCE_CONFIG_INFO));
	
	return true;
}

//-------------------------------------------------------------------------------------------------------------------------

int SZ_ProfileSetting::GetVideoEncoderConfigCount()
{
	return m_encoder_config_info.size();
}

int SZ_ProfileSetting::GetCompatibleVideoEncoderConfigCount(int width, int height)
{
	int count = 0;
	
	for(std::map<std::string, ONVIF_VIDEO_ENCODER_CONFIG_INFO>::iterator it = m_encoder_config_info.begin(); it != m_encoder_config_info.end(); it++)
	{
		if ((it->second).width <= width && (it->second).height <= height)
		{
			count ++;
		}
	}
	
	return count;
}

bool SZ_ProfileSetting::GetVideoEncoderConfig(int index, ONVIF_VIDEO_ENCODER_CONFIG_INFO* video_encoder_config_info)
{
	if (m_encoder_config_info.size() <= index)
	{
		return false;
	}
	
	std::map<std::string, ONVIF_VIDEO_ENCODER_CONFIG_INFO>::iterator it = m_encoder_config_info.begin();
	for(int i = 0; i < index; i++)
	{
		it ++;
	}
	memcpy(video_encoder_config_info, &(it->second), sizeof(ONVIF_VIDEO_ENCODER_CONFIG_INFO));
	
	return true;
}

bool SZ_ProfileSetting::FindVideoEncoderConfig(const char* token, ONVIF_VIDEO_ENCODER_CONFIG_INFO* video_encoder_config_info)
{
	DBG_TRACE("Enter SZ_ProfileSetting::FindVideoEncoderConfig");
	
	std::map<std::string, ONVIF_VIDEO_ENCODER_CONFIG_INFO>::iterator it = m_encoder_config_info.find(token);
	if (it == m_encoder_config_info.end())
	{
		DBG_PRINT("Error: NotFound");
		return false;
	}
	
	memcpy(video_encoder_config_info, &(it->second), sizeof(ONVIF_VIDEO_ENCODER_CONFIG_INFO));
	
	DBG_TRACE("Exit SZ_ProfileSetting::FindVideoEncoderConfig");
	
	return true;
}

bool SZ_ProfileSetting::ChangeVideoEncoderConfig(const char* token, int quality)
{
	std::map<std::string, ONVIF_VIDEO_ENCODER_CONFIG_INFO>::iterator it = m_encoder_config_info.find(token);
	if (it == m_encoder_config_info.end())
	{
		DBG_PRINT("Error: NotFound");
		return false;
	}
	
	(it->second).quality = quality;
	
	return OnVideoEncoderUpdate(token, &(it->second));
}

//-------------------------------------------------------------------------------------------------------------------------

int SZ_ProfileSetting::GetPtzConfigCount()
{
	return m_ptz_config_info.size();
}

bool SZ_ProfileSetting::GetPtzConfig(int index, ONVIF_PTZ_CONFIG_INFO* ptz_config_info)
{
	if (m_ptz_config_info.size() <= index)
	{
		return false;
	}
	
	std::map<std::string, ONVIF_PTZ_CONFIG_INFO>::iterator it = m_ptz_config_info.begin();
	for(int i = 0; i < index; i++)
	{
		it ++;
	}
	memcpy(ptz_config_info, &(it->second), sizeof(ONVIF_PTZ_CONFIG_INFO));
	
	return true;
}

bool SZ_ProfileSetting::FindPtzConfig(const char* token, ONVIF_PTZ_CONFIG_INFO* ptz_config_info)
{
	DBG_TRACE("Enter SZ_ProfileSetting::FindPtzConfig");
	
	std::map<std::string, ONVIF_PTZ_CONFIG_INFO>::iterator it = m_ptz_config_info.find(token);
	if (it == m_ptz_config_info.end())
	{
		DBG_PRINT("Error: NotFound");
		return false;
	}
	
	memcpy(ptz_config_info, &(it->second), sizeof(ONVIF_PTZ_CONFIG_INFO));
	
	DBG_TRACE("Exit SZ_ProfileSetting::FindPtzConfig");
	
	return true;
}

bool SZ_ProfileSetting::OnPtzUpdate(const char* token, ONVIF_PTZ_CONFIG_INFO* ptz_config_info)
{
}

//-------------------------------------------------------------------------------------------------------------------------

int SZ_ProfileSetting::GetProfileCount()
{
	return m_profile_info.size();
}

bool SZ_ProfileSetting::GetProfile(int index, ONVIF_PROFILE_INFO* profile_info)
{
	if (m_profile_info.size() <= index)
	{
		return false;
	}
	
	memcpy(profile_info, &m_profile_info[index], sizeof(ONVIF_PROFILE_INFO));
	
	return true;
}

bool SZ_ProfileSetting::FindProfile(const char* token, ONVIF_PROFILE_INFO* profile_info)
{
	DBG_TRACE("Enter SZ_ProfileSetting::FindProfile");
	DBG_PRINT("token = %s", token);
	
	for(int i = 0; i < m_profile_info.size(); i++)
	{
		if (strcmp(token, m_profile_info[i].token) == 0)
		{
			*profile_info = m_profile_info[i];
			DBG_TRACE("Exit SZ_ProfileSetting::FindProfile");
			return true;
		}
	}
	
	DBG_PRINT("Warnning: Not found profile");

	return false;
}

/**
 * プロファイル作成
 */
bool SZ_ProfileSetting::CreateProfile(const char* name, const char* token, ONVIF_PROFILE_INFO* profile_info)
{
	if (token != NULL)
	{
		if (FindProfile(token, profile_info))
		{
			return false;
		}
		strcpy(profile_info->token, token);
	}
	else
	{
		uuid_t uuid;
		uuid_generate(uuid);
		uuid_unparse(uuid, profile_info->token);
	}
	
	strcpy(profile_info->name, name);
	strcpy(profile_info->video_source_config_token, "");
	strcpy(profile_info->video_encoder_token,       "");
	profile_info->fixed = 0;
	
	m_profile_info.push_back(*profile_info);
	
	return OnProfileUpdate(token);
}

bool SZ_ProfileSetting::SetProfile(const ONVIF_PROFILE_INFO* profile_info)
{
	std::vector<ONVIF_PROFILE_INFO>::iterator it;
	for(it = m_profile_info.begin(); it != m_profile_info.end(); it++)
	{
		if (strcmp(it->token, profile_info->token) == 0)
		{
			break;
		}
	}
	if (it == m_profile_info.end())
	{
		return false;
	}
	
	*it = *profile_info;
	
	return OnProfileUpdate(profile_info->token);
}

/**
 * プロファイル削除
 */
bool SZ_ProfileSetting::DeleteProfile(const char* token)
{
	std::vector<ONVIF_PROFILE_INFO>::iterator it;
	for(it = m_profile_info.begin(); it != m_profile_info.end(); it++)
	{
		if (strcmp(it->token, token) == 0)
		{
			break;
		}
	}
	if (it == m_profile_info.end())
	{
		return false;
	}
	if (it->fixed == 1)
	{
		return false;
	}
	
	m_profile_info.erase(it);
	
	return OnProfileUpdate(token);
}

bool SZ_ProfileSetting::SetCaptureDeviceId(const char* profile_token, const char* capture_device_id)
{
	for(int i = 0; i < m_profile_info.size(); i++)
	{
		if (strcmp(profile_token, m_profile_info[i].token) == 0)
		{
			strcpy(m_profile_info[i].capture_device_id, capture_device_id);
			break;
		}
	}
	return true;
}

//-------------------------------------------------------------------------------------------------------------------------

bool SZ_ProfileSetting::AddVideoSource(const char* profile_token, const char* source_token)
{
	std::vector<ONVIF_PROFILE_INFO>::iterator it;
	for(it = m_profile_info.begin(); it != m_profile_info.end(); it++)
	{
		if (strcmp(it->token, profile_token) == 0)
		{
			break;
		}
	}
	if (it == m_profile_info.end())
	{
		DBG_PRINT("Error: NotFound");
		return false;
	}
	
	std::map<std::string, ONVIF_VIDEO_SOURCE_CONFIG_INFO>::iterator it1 = m_source_config_info.find(source_token);
	if (it1 == m_source_config_info.end())
	{
		DBG_PRINT("Error: NotFound VideoSource");
		return false;
	}
	
	strcpy(it->video_source_config_token, source_token);

	return OnProfileUpdate(profile_token);
}

bool SZ_ProfileSetting::RemoveVideoSource(const char* profile_token)
{
	std::vector<ONVIF_PROFILE_INFO>::iterator it;
	for(it = m_profile_info.begin(); it != m_profile_info.end(); it++)
	{
		if (strcmp(it->token, profile_token) == 0)
		{
			break;
		}
	}
	if (it == m_profile_info.end())
	{
		DBG_PRINT("Error: NotFound");
		return false;
	}
	
	strcpy(it->video_encoder_token, "");
	
	return OnProfileUpdate(profile_token);
}

//-------------------------------------------------------------------------------------------------------------------------

bool SZ_ProfileSetting::AddVideoEncoder(const char* profile_token, const char* encoder_token)
{
	std::vector<ONVIF_PROFILE_INFO>::iterator it;
	for(it = m_profile_info.begin(); it != m_profile_info.end(); it++)
	{
		if (strcmp(it->token, profile_token) == 0)
		{
			break;
		}
	}
	if (it == m_profile_info.end())
	{
		DBG_PRINT("Error: NotFound");
		return false;
	}
	
	std::map<std::string, ONVIF_VIDEO_ENCODER_CONFIG_INFO>::iterator it1 = m_encoder_config_info.find(encoder_token);
	if (it1 == m_encoder_config_info.end())
	{
		DBG_PRINT("Error: NotFound VideoEncoder");
		return false;
	}
	
	strcpy(it->video_encoder_token, encoder_token);
	
	return OnProfileUpdate(profile_token);
}

bool SZ_ProfileSetting::RemoveVideoEncoder(const char* profile_token)
{
	std::vector<ONVIF_PROFILE_INFO>::iterator it;
	for(it = m_profile_info.begin(); it != m_profile_info.end(); it++)
	{
		if (strcmp(it->token, profile_token) == 0)
		{
			break;
		}
	}
	if (it == m_profile_info.end())
	{
		DBG_PRINT("Error: NotFound");
		return false;
	}
	
	strcpy(it->video_encoder_token, "");
	
	return OnProfileUpdate(profile_token);
}

//-------------------------------------------------------------------------------------------------------------------------

