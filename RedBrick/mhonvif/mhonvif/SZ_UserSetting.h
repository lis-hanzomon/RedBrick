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

#ifndef SZ_USER_SETTING_H
#define SZ_USER_SETTING_H

#include "mhengine/MH_CriticalSection.h"
#include "SZ_OnvifInfo.h"

#include <map>
#include <string>

class SZ_UserSetting
{
protected:
	MH_CriticalSection m_sync;
	std::map<std::string, ONVIF_USER_INFO> m_user_info;

	virtual bool OnAddUserInfo(const ONVIF_USER_INFO* user_info);
	virtual bool OnSetUserInfo(const ONVIF_USER_INFO* user_info);
	virtual bool OnDeleteUserInfo(const char* username);
	
public:
	SZ_UserSetting();
	virtual ~SZ_UserSetting();
	
	bool AddUserInfo(const ONVIF_USER_INFO* user_info);
	bool SetUserInfo(const ONVIF_USER_INFO* user_info);
	bool DeleteUserInfo(const char* username);
	
	int GetUserCount();
	bool GetUserInfo(int index, ONVIF_USER_INFO* user_info);
	
	bool FindUserInfo(const char* username, ONVIF_USER_INFO* user_info);
};

#endif
