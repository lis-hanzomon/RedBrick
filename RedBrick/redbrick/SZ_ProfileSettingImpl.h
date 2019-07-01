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

#ifndef SZ_PROFILE_SETTING_IMPL_H
#define SZ_PROFILE_SETTING_IMPL_H

#include "mhonvif/SZ_ProfileSetting.h"

class SZ_ProfileSettingImpl : public SZ_ProfileSetting
{
private:
	char m_path[255];
	
	void LoadVideoSourceConfig();
	void LoadVideoEncoderConfig();
	void LoadPtzConfig();
	void LoadProfile();
	bool SaveProfile();

protected:
	virtual bool OnVideoEncoderUpdate(const char* token, ONVIF_VIDEO_ENCODER_CONFIG_INFO* encoder_config_info);
	virtual bool OnProfileUpdate(const char* token);

public:
	SZ_ProfileSettingImpl();
	virtual ~SZ_ProfileSettingImpl();

	bool Initialize(const char* path, bool is_ptz_support);

	virtual int GetSupportedResolutionCount();
	virtual bool GetSupportedResolution(int index, int* width, int* height);
};

#endif
