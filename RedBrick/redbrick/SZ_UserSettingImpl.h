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

#ifndef SZ_USER_SETTING_IMPL_H
#define SZ_USER_SETTING_IMPL_H

#include "mhonvif/SZ_UserSetting.h"

class SZ_UserSettingImpl : public SZ_UserSetting
{
private:
	char m_path[255];

	bool Load();
	bool Save();
	
protected:
	virtual bool OnAddUserInfo(const ONVIF_USER_INFO* user_info);
	virtual bool OnSetUserInfo(const ONVIF_USER_INFO* user_info);
	virtual bool OnDeleteUserInfo(const char* username);

public:
	SZ_UserSettingImpl();
	virtual ~SZ_UserSettingImpl();

	bool Initialize(const char* path);
};

#endif
