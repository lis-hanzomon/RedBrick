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
#include "SZ_UserSettingImpl.h"
#include <json-c/json.h>

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

SZ_UserSettingImpl::SZ_UserSettingImpl()
{
}

SZ_UserSettingImpl::~SZ_UserSettingImpl()
{
}

bool SZ_UserSettingImpl::Initialize(const char* path)
{
	sprintf(m_path, "%s/user_info.json", path);
	
	return Load();
}

bool SZ_UserSettingImpl::Load()
{
	DBG_TRACE("Enter SZ_UserSettingImpl::Load");
	
	struct json_object* jo = json_object_from_file(m_path);
	if (jo == NULL)
	{
		return false;
	}

	json_object_object_foreach(jo, key, val)
	{
		if(json_object_is_type(val, json_type_array))
		{
			if (strcmp(key, "Users") == 0)
			{
				int len = json_object_array_length(val);
				for (int i = 0; i < len; i++)
				{
				 	ONVIF_USER_INFO user_info;
				 	memset(&user_info, 0x00, sizeof(user_info));
					
					struct json_object * users = json_object_array_get_idx(val, i);
					json_object_object_foreach(users, users_key, users_val)
					{
						if(json_object_is_type(users_val, json_type_string))
						{
							if (strcmp(users_key, "Username") == 0)
							{
								strcpy(user_info.username, json_object_get_string(users_val));
							}
							else if (strcmp(users_key, "Password") == 0)
							{
								strcpy(user_info.password, json_object_get_string(users_val));
							}
						}
						else if(json_object_is_type(users_val, json_type_int))
						{
							if (strcmp(users_key, "Level") == 0)
							{
								user_info.level = json_object_get_int(users_val);
							}
   					 	}
   					 }
   					 
   					 DBG_PRINT("username = %s", user_info.username);
   					 DBG_PRINT("password = %s", user_info.password);
   					 DBG_PRINT("level    = %d", user_info.level);
   					 
   					 m_user_info[user_info.username] = user_info;
				 }
			}
		}
	}
	
	DBG_TRACE("Exit SZ_UserSettingImpl::Load");
	
	return true;
}

bool SZ_UserSettingImpl::Save()
{
	DBG_TRACE("Enter SZ_UserSettingImpl::Save");
	
	FILE* fp = fopen(m_path, "w");
	if (fp == NULL)
	{
		DBG_PRINT("Error: fopen = NULL");
		return false;
	}
	
	fprintf(
		fp, 
		"{\n"
		"\t\"Users\" : [\n"
		);
	
	std::map<std::string, ONVIF_USER_INFO>::iterator it = m_user_info.begin();
	
	for(int i = 0; i < m_user_info.size(); i++)
	{
		fprintf(
			fp,
			"\t\t{\n"
			"\t\t\t\"Username\" : \"%s\",\n"
			"\t\t\t\"Password\" : \"%s\",\n"
			"\t\t\t\"Level\" : %d\n",
			(it->second).username,
			(it->second).password,
			(it->second).level
			);
			
		if (i + 1 < m_user_info.size())
		{
			fprintf(fp, "\t\t},\n");
		}
		else
		{
			fprintf(fp, "\t\t}\n");
		}
		
		it++;
	}
	
	fprintf(
		fp, 
		"\t]\n"
		"}\n"
		);
	
	fclose(fp);
	
	DBG_TRACE("Exit SZ_UserSettingImpl::Save");
	
	return true;
}
	
bool SZ_UserSettingImpl::OnAddUserInfo(const ONVIF_USER_INFO* user_info)
{
	return Save();
}

bool SZ_UserSettingImpl::OnSetUserInfo(const ONVIF_USER_INFO* user_info)
{
	return Save();
}

bool SZ_UserSettingImpl::OnDeleteUserInfo(const char* username)
{
	return Save();
}
