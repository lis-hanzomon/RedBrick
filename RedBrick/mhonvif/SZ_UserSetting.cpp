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
#include "mhonvif/SZ_UserSetting.h"

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

SZ_UserSetting::SZ_UserSetting()
{
}

SZ_UserSetting::~SZ_UserSetting()
{
}

bool SZ_UserSetting::AddUserInfo(const ONVIF_USER_INFO* user_info)
{
	DBG_TRACE("Enter SZ_UserSetting::AddUserInfo");
	
	{
		MH_AutoLock lock(&m_sync);
	
		std::map<std::string, ONVIF_USER_INFO>::iterator it = m_user_info.find(user_info->username);
		if (it != m_user_info.end())
		{
			return false;
		}
		
		m_user_info[user_info->username] = *user_info;
	
		if (!OnAddUserInfo(user_info))
		{
			return false;
		}
	}
	
	DBG_TRACE("Exit SZ_UserSetting::AddUserInfo");
	
	return true;
}

bool SZ_UserSetting::SetUserInfo(const ONVIF_USER_INFO* user_info)
{
	DBG_TRACE("Enter SZ_UserSetting::SetUserInfo");
	
	{
		MH_AutoLock lock(&m_sync);
	
		std::map<std::string, ONVIF_USER_INFO>::iterator it = m_user_info.find(user_info->username);
		if (it == m_user_info.end())
		{
			return false;
		}
		
		struct ONVIF_USER_INFO& target_user_info = it->second;
		
		strcpy(target_user_info.password, user_info->password);
		target_user_info.level = user_info->level;
		
		if (!OnSetUserInfo(user_info))
		{
			return false;
		}
	}
	
	DBG_TRACE("Exit SZ_UserSetting::SetUserInfo");
	
	return true;
}

bool SZ_UserSetting::DeleteUserInfo(const char* username)
{
	DBG_TRACE("Enter SZ_UserSetting::DeleteUserInfo");
	
	{
		MH_AutoLock lock(&m_sync);
		
		std::map<std::string, ONVIF_USER_INFO>::iterator it = m_user_info.find(username);
		if (it == m_user_info.end())
		{
			return false;
		}
		
		m_user_info.erase(username);
		
		if (!OnDeleteUserInfo(username))
		{
			return false;
		}
	}
	
	DBG_TRACE("Exit SZ_UserSetting::DeleteUserInfo");
	
	return true;
}

bool SZ_UserSetting::OnAddUserInfo(const ONVIF_USER_INFO* user_info)
{
	return true;
}

bool SZ_UserSetting::OnSetUserInfo(const ONVIF_USER_INFO* user_info)
{
	return true;
}

bool SZ_UserSetting::OnDeleteUserInfo(const char* username)
{
	return true;
}

int SZ_UserSetting::GetUserCount()
{
	{
		MH_AutoLock lock(&m_sync);

		return m_user_info.size();
	}
}

bool SZ_UserSetting::GetUserInfo(int index, ONVIF_USER_INFO* user_info)
{
	{
		MH_AutoLock lock(&m_sync);
		
		std::map<std::string, ONVIF_USER_INFO>::iterator it = m_user_info.begin();
		for(int i = 0; i < index; i++)
		{
			it ++;
		}
		memcpy(user_info, &(it->second), sizeof(ONVIF_USER_INFO));
	}
	
	return true;
}

bool SZ_UserSetting::FindUserInfo(const char* username, ONVIF_USER_INFO* user_info)
{
	{
		MH_AutoLock lock(&m_sync);

		std::map<std::string, ONVIF_USER_INFO>::iterator it = m_user_info.find(username);
		if (it == m_user_info.end())
		{
			return false;
		}
		memcpy(user_info, &(it->second), sizeof(ONVIF_USER_INFO));
	}
	
	return true;
}
