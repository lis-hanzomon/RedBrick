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

#ifndef SZ_PROFILE_SETTING_H
#define SZ_PROFILE_SETTING_H

#include "SZ17.h"

#include "SZ_OnvifInfo.h"

#include <map>
#include <vector>
#include <string>

class SZ17_API SZ_ProfileSetting
{
protected:
	std::map<std::string, ONVIF_VIDEO_SOURCE_CONFIG_INFO> m_source_config_info;
	std::map<std::string, ONVIF_VIDEO_ENCODER_CONFIG_INFO> m_encoder_config_info;
	std::map<std::string, ONVIF_PTZ_CONFIG_INFO> m_ptz_config_info;
	std::vector<ONVIF_PROFILE_INFO> m_profile_info;

	virtual bool OnVideoEncoderUpdate(const char* token, ONVIF_VIDEO_ENCODER_CONFIG_INFO* encoder_config_info) = 0;
	virtual bool OnPtzUpdate(const char* token, ONVIF_PTZ_CONFIG_INFO* ptz_config_info);
	virtual bool OnProfileUpdate(const char* token) = 0;

public:
	SZ_ProfileSetting();
	virtual ~SZ_ProfileSetting();
	
	int GetVideoSourceConfigCount();
	int GetCompatibleVideoSourceConfigCount(int width, int height);
	bool GetVideoSourceConfig(int index, ONVIF_VIDEO_SOURCE_CONFIG_INFO* video_source_config_info);
	bool FindVideoSourceConfig(const char* token, ONVIF_VIDEO_SOURCE_CONFIG_INFO* video_source_config_info);

	int GetVideoEncoderConfigCount();
	int GetCompatibleVideoEncoderConfigCount(int width, int height);
	bool GetVideoEncoderConfig(int index, ONVIF_VIDEO_ENCODER_CONFIG_INFO* video_encoder_config_info);
	bool FindVideoEncoderConfig(const char* token, ONVIF_VIDEO_ENCODER_CONFIG_INFO* video_encoder_config_info);
	bool ChangeVideoEncoderConfig(const char* encoder_token, int quality);

	int GetPtzConfigCount();
	bool GetPtzConfig(int index, ONVIF_PTZ_CONFIG_INFO* ptz_config_info);
	bool FindPtzConfig(const char* token, ONVIF_PTZ_CONFIG_INFO* ptz_config_info);

	virtual int GetSupportedResolutionCount() = 0;
	virtual bool GetSupportedResolution(int index, int* width, int* height) = 0;

	int GetProfileCount();
	bool GetProfile(int index, ONVIF_PROFILE_INFO* profile_info);
	bool FindProfile(const char* token, ONVIF_PROFILE_INFO* profile_info);
	bool CreateProfile(const char* name, const char* token, ONVIF_PROFILE_INFO* profile_info);
	bool SetProfile(const ONVIF_PROFILE_INFO* profile_info);
	bool DeleteProfile(const char* token);

	bool SetCaptureDeviceId(const char* profile_token, const char* capture_device_id);

	bool AddVideoSource(const char* profile_token, const char* source_token);
	bool RemoveVideoSource(const char* profile_token);

	bool AddVideoEncoder(const char* profile_token, const char* encoder_token);
	bool RemoveVideoEncoder(const char* profile_token);
};

#endif
