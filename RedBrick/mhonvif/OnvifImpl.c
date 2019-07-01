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

#include <stdio.h>
#include <pthread.h>
#include "soapH.h"
#include "MediaBinding.nsmap"

#include "wsseapi.h"
#include "wsaapi.h" /* from plugin/wsaapi.h */

#include "OnvifImpl.h"
#include "OnvifBridge.h"

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

//-------------------------------------------------------------------------------------------------------------

int VerifyPassword(struct soap* soap, int level)
{
	struct ONVIF_USER_INFO user_info;
	memset(&user_info, 0x00, sizeof(user_info));
	
	const char* username = soap_wsse_get_Username(soap);
	if (username != NULL)
	{
		if (Bridge_FindUserInfo(username, &user_info) != 0)
		{
			return SOAP_ERR;
		}
	}
	
	if (soap_wsse_verify_Password(soap, user_info.password))
	{
		soap_wsse_delete_Security(soap);
		return soap->error;
	}
	
	if (level < user_info.level)
	{
		return SOAP_ERR;
	}

	return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// #### DEVICE ####
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/** Web service operation '__tds__GetServices' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetServices(struct soap* soap, struct _tds__GetServices *tds__GetServices, struct _tds__GetServicesResponse *tds__GetServicesResponse)
{
	DBG_TRACE("### __tds__GetServices ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetServiceCapabilities(struct soap* soap, struct _tds__GetServiceCapabilities *tds__GetServiceCapabilities, struct _tds__GetServiceCapabilitiesResponse *tds__GetServiceCapabilitiesResponse)
{
	DBG_TRACE("### __tds__GetServiceCapabilities ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDeviceInformation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDeviceInformation(struct soap* soap, struct _tds__GetDeviceInformation *tds__GetDeviceInformation, struct _tds__GetDeviceInformationResponse *tds__GetDeviceInformationResponse)
{
	DBG_TRACE("### __tds__GetDeviceInformation ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	struct ONVIF_DEVICE_INFO device_info;
	Bridge_GetDeviceInfo(&device_info);

	tds__GetDeviceInformationResponse->Manufacturer    = soap_strdup(soap, device_info.Manufacturer);
	tds__GetDeviceInformationResponse->Model           = soap_strdup(soap, device_info.Model);
	tds__GetDeviceInformationResponse->FirmwareVersion = soap_strdup(soap, device_info.FirmwareVersion);
	tds__GetDeviceInformationResponse->SerialNumber    = soap_strdup(soap, device_info.SerialNumber);
	tds__GetDeviceInformationResponse->HardwareId      = soap_strdup(soap, device_info.HardwareId);
	
	return 0;
}

/** Web service operation '__tds__SetSystemDateAndTime' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetSystemDateAndTime(struct soap* soap, struct _tds__SetSystemDateAndTime *tds__SetSystemDateAndTime, struct _tds__SetSystemDateAndTimeResponse *tds__SetSystemDateAndTimeResponse)
{
	DBG_TRACE("### __tds__SetSystemDateAndTime ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	if (tds__SetSystemDateAndTime->TimeZone != NULL)
	{
		if (tds__SetSystemDateAndTime->TimeZone->TZ != NULL)
		{
			DBG_PRINT("TimeZone = %s", tds__SetSystemDateAndTime->TimeZone->TZ);
			
			// タイムゾーン変更は未サポート
		}
	}
	
	DBG_PRINT("DaylightSavings = %d", tds__SetSystemDateAndTime->DaylightSavings);
	DBG_PRINT("DateTimeType    = %d", tds__SetSystemDateAndTime->DateTimeType);
	
	if (tds__SetSystemDateAndTime->DateTimeType == tt__SetDateTimeType__Manual)
	{
		//--------------------------------------------------------------------------------------------
		// 指定された時刻に設定する
		//--------------------------------------------------------------------------------------------

		if (tds__SetSystemDateAndTime->UTCDateTime != NULL)
		{
			if (tds__SetSystemDateAndTime->UTCDateTime->Time != NULL && 
				tds__SetSystemDateAndTime->UTCDateTime->Date != NULL)
			{
				DBG_PRINT("### Manual ###");
		
				DBG_PRINT("Time = %d:%d:%d", 
					tds__SetSystemDateAndTime->UTCDateTime->Time->Hour,
					tds__SetSystemDateAndTime->UTCDateTime->Time->Minute,
					tds__SetSystemDateAndTime->UTCDateTime->Time->Second
				);
				DBG_PRINT("Date = %d/%d/%d", 
					tds__SetSystemDateAndTime->UTCDateTime->Date->Year,
					tds__SetSystemDateAndTime->UTCDateTime->Date->Month,
					tds__SetSystemDateAndTime->UTCDateTime->Date->Day
				);
				
				struct tm utc_tm;
				utc_tm.tm_sec  = tds__SetSystemDateAndTime->UTCDateTime->Time->Second;
				utc_tm.tm_min  = tds__SetSystemDateAndTime->UTCDateTime->Time->Minute;
				utc_tm.tm_hour = tds__SetSystemDateAndTime->UTCDateTime->Time->Hour;
				utc_tm.tm_mday = tds__SetSystemDateAndTime->UTCDateTime->Date->Day;
				utc_tm.tm_mon  = tds__SetSystemDateAndTime->UTCDateTime->Date->Month - 1;
				utc_tm.tm_year = tds__SetSystemDateAndTime->UTCDateTime->Date->Year - 1900;
				
				struct timeval tv;
				tv.tv_sec = timegm(&utc_tm);
				tv.tv_usec = 0;
				settimeofday(&tv, NULL);
			}
		}
	}
	else if (tds__SetSystemDateAndTime->DateTimeType == tt__SetDateTimeType__NTP)
	{
		DBG_PRINT("### NTP ###");
		
		// NTPは未サポート
	}
	
	return 0;
}

/** Web service operation '__tds__GetSystemDateAndTime' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemDateAndTime(struct soap* soap, struct _tds__GetSystemDateAndTime *tds__GetSystemDateAndTime, struct _tds__GetSystemDateAndTimeResponse *tds__GetSystemDateAndTimeResponse)
{
	DBG_TRACE("### __tds__GetSystemDateAndTime ###");
	
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	struct tm tm;
	gmtime_r(&tv.tv_sec, &tm);
	DBG_PRINT(
		"%04d/%02d/%02d %02d:%02d:%02d",
		tm.tm_year + 1900, 
		tm.tm_mon + 1, 
		tm.tm_mday,
		tm.tm_hour, 
		tm.tm_min,
		tm.tm_sec
	);
	
	//----------------------------------------------
	// ユーザー認証は実行しない
	// ※認証を行う為に、デバイスの日時が必要
	//----------------------------------------------
	
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime = soap_new_tt__SystemDateTime(soap, 1);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DateTimeType = tt__SetDateTimeType__Manual;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DaylightSavings = xsd__boolean__false_;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime = soap_new_tt__DateTime(soap, 1);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time = soap_new_tt__Time(soap, 1);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Hour   = tm.tm_hour;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Minute = tm.tm_min;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Second = tm.tm_sec;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date = soap_new_tt__Date(soap, 1);
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Year  = tm.tm_year + 1900;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Month = tm.tm_mon + 1;
	tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Day   = tm.tm_mday;
	
	return 0;
}

/** Web service operation '__tds__SetSystemFactoryDefault' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetSystemFactoryDefault(struct soap* soap, struct _tds__SetSystemFactoryDefault *tds__SetSystemFactoryDefault, struct _tds__SetSystemFactoryDefaultResponse *tds__SetSystemFactoryDefaultResponse)
{
	DBG_TRACE("### __tds__SetSystemFactoryDefault ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	if (tds__SetSystemFactoryDefault->FactoryDefault == tt__FactoryDefaultType__Soft)
	{
		Bridge_FactoryReset(0);
	}
	else if (tds__SetSystemFactoryDefault->FactoryDefault == tt__FactoryDefaultType__Hard)
	{
		Bridge_FactoryReset(1);
	}
	
	return 0;
}

/** Web service operation '__tds__UpgradeSystemFirmware' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__UpgradeSystemFirmware(struct soap* soap, struct _tds__UpgradeSystemFirmware *tds__UpgradeSystemFirmware, struct _tds__UpgradeSystemFirmwareResponse *tds__UpgradeSystemFirmwareResponse)
{
	DBG_TRACE("### __tds__UpgradeSystemFirmware ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SystemReboot' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SystemReboot(struct soap* soap, struct _tds__SystemReboot *tds__SystemReboot, struct _tds__SystemRebootResponse *tds__SystemRebootResponse)
{
	DBG_TRACE("### __tds__SystemReboot ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	Bridge_SystemReboot();
	
	tds__SystemRebootResponse->Message = strdup("will be reboot soon.");

	return 0;
}

/** Web service operation '__tds__RestoreSystem' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RestoreSystem(struct soap* soap, struct _tds__RestoreSystem *tds__RestoreSystem, struct _tds__RestoreSystemResponse *tds__RestoreSystemResponse)
{
	DBG_TRACE("### __tds__RestoreSystem ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetSystemBackup' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemBackup(struct soap* soap, struct _tds__GetSystemBackup *tds__GetSystemBackup, struct _tds__GetSystemBackupResponse *tds__GetSystemBackupResponse)
{
	DBG_TRACE("### __tds__GetSystemBackup ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetSystemLog' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemLog(struct soap* soap, struct _tds__GetSystemLog *tds__GetSystemLog, struct _tds__GetSystemLogResponse *tds__GetSystemLogResponse)
{
	DBG_TRACE("### __tds__GetSystemLog ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("LogType = %d", tds__GetSystemLog->LogType);
	
	if (tds__GetSystemLog->LogType == tt__SystemLogType__System)
	{
		tds__GetSystemLogResponse->SystemLog = soap_new_tt__SystemLog(soap, 1);
		tds__GetSystemLogResponse->SystemLog->String = soap_strdup(soap, "test1\n");
	}
	else if (tds__GetSystemLog->LogType == tt__SystemLogType__Access)
	{
		tds__GetSystemLogResponse->SystemLog = soap_new_tt__SystemLog(soap, 1);
		tds__GetSystemLogResponse->SystemLog->String = soap_strdup(soap, "test2\n");
	}
	
	return 0;
}

/** Web service operation '__tds__GetSystemSupportInformation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemSupportInformation(struct soap* soap, struct _tds__GetSystemSupportInformation *tds__GetSystemSupportInformation, struct _tds__GetSystemSupportInformationResponse *tds__GetSystemSupportInformationResponse)
{
	DBG_TRACE("### __tds__GetSystemSupportInformation ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetScopes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetScopes(struct soap* soap, struct _tds__GetScopes *tds__GetScopes, struct _tds__GetScopesResponse *tds__GetScopesResponse)
{
	DBG_TRACE("### __tds__GetScopes ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	char buff[256];
	
	tds__GetScopesResponse->__sizeScopes = 6;
	tds__GetScopesResponse->Scopes = soap_new_tt__Scope(soap, 6);
	
	tds__GetScopesResponse->Scopes[0].ScopeDef = tt__ScopeDefinition__Fixed;
	tds__GetScopesResponse->Scopes[0].ScopeItem = soap_strdup(soap, "onvif://www.onvif.org/type/Network_Transmitter");
	tds__GetScopesResponse->Scopes[1].ScopeDef = tt__ScopeDefinition__Fixed;
	tds__GetScopesResponse->Scopes[1].ScopeItem = soap_strdup(soap, "onvif://www.onvif.org/type/video_encoder");
	
	struct ONVIF_DEVICE_INFO device_info;
	Bridge_GetDeviceInfo(&device_info);
	
	sprintf(buff, "onvif://www.onvif.org/hardware/%s", device_info.Model);
	tds__GetScopesResponse->Scopes[2].ScopeDef = tt__ScopeDefinition__Fixed;
	tds__GetScopesResponse->Scopes[2].ScopeItem = soap_strdup(soap, buff);
	
	tds__GetScopesResponse->Scopes[3].ScopeDef = tt__ScopeDefinition__Fixed;
	tds__GetScopesResponse->Scopes[3].ScopeItem = soap_strdup(soap, "onvif://www.onvif.org/Profile/Streaming");
	
	char name[256];
	char location[256];
	Bridge_GetOnvifIdntification(name, location);
	
	sprintf(buff, "onvif://www.onvif.org/name/%s", name);
	tds__GetScopesResponse->Scopes[4].ScopeDef = tt__ScopeDefinition__Configurable;
	tds__GetScopesResponse->Scopes[4].ScopeItem = soap_strdup(soap, buff);
	
	sprintf(buff, "onvif://www.onvif.org/location/%s", location);
	tds__GetScopesResponse->Scopes[5].ScopeDef = tt__ScopeDefinition__Configurable;
	tds__GetScopesResponse->Scopes[5].ScopeItem = soap_strdup(soap, buff);
	
	return 0;
}

/** Web service operation '__tds__SetScopes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetScopes(struct soap* soap, struct _tds__SetScopes *tds__SetScopes, struct _tds__SetScopesResponse *tds__SetScopesResponse)
{
	DBG_TRACE("### __tds__SetScopes ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}

	char name[256];
	char location[256];
	Bridge_GetOnvifIdntification(name, location);

	DBG_PRINT("__sizeScopes = %d", tds__SetScopes->__sizeScopes);
	int i;
	for(i = 0; i < tds__SetScopes->__sizeScopes; i++)
	{
		DBG_PRINT("Scopes = %s", tds__SetScopes->Scopes[i]);
		
		if (strncmp(tds__SetScopes->Scopes[i], "onvif://www.onvif.org/name/", strlen("onvif://www.onvif.org/name/")) == 0)
		{
			strcpy(name, tds__SetScopes->Scopes[i] + strlen("onvif://www.onvif.org/name/"));
		}
		else if (strncmp(tds__SetScopes->Scopes[i], "onvif://www.onvif.org/location/", strlen("onvif://www.onvif.org/location/")) == 0)
		{
			strcpy(location, tds__SetScopes->Scopes[i] + strlen("onvif://www.onvif.org/location/"));
		}
	}
	
	Bridge_SetOnvifIdntification(name, location);
	
	return 0;
}

/** Web service operation '__tds__AddScopes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__AddScopes(struct soap* soap, struct _tds__AddScopes *tds__AddScopes, struct _tds__AddScopesResponse *tds__AddScopesResponse)
{
	DBG_TRACE("### __tds__SetScopes ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__RemoveScopes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RemoveScopes(struct soap* soap, struct _tds__RemoveScopes *tds__RemoveScopes, struct _tds__RemoveScopesResponse *tds__RemoveScopesResponse)
{
	DBG_TRACE("### __tds__SetScopes ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDiscoveryMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDiscoveryMode(struct soap* soap, struct _tds__GetDiscoveryMode *tds__GetDiscoveryMode, struct _tds__GetDiscoveryModeResponse *tds__GetDiscoveryModeResponse)
{
	DBG_TRACE("### __tds__GetDiscoveryMode ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	if (Bridge_GetDiscoveryMode() == 1)
	{
		tds__GetDiscoveryModeResponse->DiscoveryMode = tt__DiscoveryMode__Discoverable;
	}
	else
	{
		tds__GetDiscoveryModeResponse->DiscoveryMode = tt__DiscoveryMode__NonDiscoverable;
	}
	
	return 0;
}

/** Web service operation '__tds__SetDiscoveryMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDiscoveryMode(struct soap* soap, struct _tds__SetDiscoveryMode *tds__SetDiscoveryMode, struct _tds__SetDiscoveryModeResponse *tds__SetDiscoveryModeResponse)
{
	DBG_TRACE("### __tds__SetDiscoveryMode ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("DiscoveryMode = %d", tds__SetDiscoveryMode->DiscoveryMode);
	
	if (tds__SetDiscoveryMode->DiscoveryMode == tt__DiscoveryMode__Discoverable)
	{
		Bridge_SetDiscoveryMode(1);
	}
	else if (tds__SetDiscoveryMode->DiscoveryMode == tt__DiscoveryMode__NonDiscoverable)
	{
		Bridge_SetDiscoveryMode(0);
	}
	
	return 0;
}

/** Web service operation '__tds__GetRemoteDiscoveryMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRemoteDiscoveryMode(struct soap* soap, struct _tds__GetRemoteDiscoveryMode *tds__GetRemoteDiscoveryMode, struct _tds__GetRemoteDiscoveryModeResponse *tds__GetRemoteDiscoveryModeResponse)
{
	DBG_TRACE("### __tds__GetRemoteDiscoveryMode ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetRemoteDiscoveryMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRemoteDiscoveryMode(struct soap* soap, struct _tds__SetRemoteDiscoveryMode *tds__SetRemoteDiscoveryMode, struct _tds__SetRemoteDiscoveryModeResponse *tds__SetRemoteDiscoveryModeResponse)
{
	DBG_TRACE("### __tds__SetRemoteDiscoveryMode ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDPAddresses' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDPAddresses(struct soap* soap, struct _tds__GetDPAddresses *tds__GetDPAddresses, struct _tds__GetDPAddressesResponse *tds__GetDPAddressesResponse)
{
	DBG_TRACE("### __tds__GetDPAddresses ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetEndpointReference' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetEndpointReference(struct soap* soap, struct _tds__GetEndpointReference *tds__GetEndpointReference, struct _tds__GetEndpointReferenceResponse *tds__GetEndpointReferenceResponse)
{
	DBG_TRACE("### __tds__GetEndpointReference ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetRemoteUser' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRemoteUser(struct soap* soap, struct _tds__GetRemoteUser *tds__GetRemoteUser, struct _tds__GetRemoteUserResponse *tds__GetRemoteUserResponse)
{
	DBG_TRACE("### __tds__GetRemoteUser ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetRemoteUser' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRemoteUser(struct soap* soap, struct _tds__SetRemoteUser *tds__SetRemoteUser, struct _tds__SetRemoteUserResponse *tds__SetRemoteUserResponse)
{
	DBG_TRACE("### __tds__SetRemoteUser ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetUsers' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetUsers(struct soap* soap, struct _tds__GetUsers *tds__GetUsers, struct _tds__GetUsersResponse *tds__GetUsersResponse)
{
	DBG_TRACE("### __tds__GetUsers ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	tds__GetUsersResponse->__sizeUser = Bridge_GetUserCount();
	if (tds__GetUsersResponse->__sizeUser > 0)
	{
		tds__GetUsersResponse->User = soap_new_tt__User(soap, tds__GetUsersResponse->__sizeUser);
		
		{
			int i;
			for(i = 0; i < tds__GetUsersResponse->__sizeUser; i++)
			{
				struct ONVIF_USER_INFO user_info;
				Bridge_GetUserInfo(i, &user_info);
				
				tds__GetUsersResponse->User[i].Username = strdup(user_info.username);
				tds__GetUsersResponse->User[i].UserLevel = user_info.level;
			}
		}
	}
	
	return 0;
}

/** Web service operation '__tds__CreateUsers' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateUsers(struct soap* soap, struct _tds__CreateUsers *tds__CreateUsers, struct _tds__CreateUsersResponse *tds__CreateUsersResponse)
{
	DBG_TRACE("### __tds__CreateUsers ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("__sizeUser = %d", tds__CreateUsers->__sizeUser);
	int i;
	for(i = 0; i < tds__CreateUsers->__sizeUser; i++)
	{
		DBG_PRINT("Username  = %s", tds__CreateUsers->User[i].Username);
		// DBG_PRINT("Password  = %s", tds__CreateUsers->User[i].Password);
		DBG_PRINT("UserLevel = %d", tds__CreateUsers->User[i].UserLevel);
		
		struct ONVIF_USER_INFO user_info;
		strcpy(user_info.username, tds__CreateUsers->User[i].Username);
		strcpy(user_info.password, tds__CreateUsers->User[i].Password);
		user_info.level = tds__CreateUsers->User[i].UserLevel;
		Bridge_AddUserInfo(&user_info);
	}

	return 0;
}

/** Web service operation '__tds__DeleteUsers' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteUsers(struct soap* soap, struct _tds__DeleteUsers *tds__DeleteUsers, struct _tds__DeleteUsersResponse *tds__DeleteUsersResponse)
{
	DBG_TRACE("### __tds__DeleteUsers ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("__sizeUser = %d", tds__DeleteUsers->__sizeUsername);
	int i;
	for(i = 0; i < tds__DeleteUsers->__sizeUsername; i++)
	{
		DBG_PRINT("Username = %s", tds__DeleteUsers->Username[i]);
		Bridge_DeleteUserInfo(tds__DeleteUsers->Username[i]);
	}

	return 0;
}

/** Web service operation '__tds__SetUser' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetUser(struct soap* soap, struct _tds__SetUser *tds__SetUser, struct _tds__SetUserResponse *tds__SetUserResponse)
{
	DBG_TRACE("### __tds__SetUser ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("__sizeUser = %d\n", tds__SetUser->__sizeUser);
	int i;
	for(i = 0; i < tds__SetUser->__sizeUser; i++)
	{
		DBG_PRINT("Username  = %s", tds__SetUser->User[i].Username);
		// DBG_PRINT("Password  = %s", tds__SetUser->User[i].Password);
		DBG_PRINT("UserLevel = %d", tds__SetUser->User[i].UserLevel);
		
		struct ONVIF_USER_INFO user_info;
		strcpy(user_info.username, tds__SetUser->User[i].Username);
		strcpy(user_info.password, tds__SetUser->User[i].Password);
		user_info.level = tds__SetUser->User[i].UserLevel;
		Bridge_SetUserInfo(&user_info);
	}
	
	return 0;
}

/** Web service operation '__tds__GetWsdlUrl' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetWsdlUrl(struct soap* soap, struct _tds__GetWsdlUrl *tds__GetWsdlUrl, struct _tds__GetWsdlUrlResponse *tds__GetWsdlUrlResponse)
{
	DBG_TRACE("### __tds__GetWsdlUrl ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCapabilities(struct soap* soap, struct _tds__GetCapabilities *tds__GetCapabilities, struct _tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse)
{
	DBG_TRACE("### __tds__GetCapabilities ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	const char* ip_address = Bridge_GetIpAddres();
	int onvif_port_no = Bridge_GetOnvifPortNo();

	DBG_PRINT("__sizeCategory = %d", tds__GetCapabilities->__sizeCategory);
	unsigned char mask = 0x00;
	int i;
	for(i = 0; i < tds__GetCapabilities->__sizeCategory; i++)
	{
		DBG_PRINT("Category = %d", tds__GetCapabilities->Category[i]);
		
		switch (tds__GetCapabilities->Category[i])
		{
			case tt__CapabilityCategory__All:		mask |= 0xff;	break;
			case tt__CapabilityCategory__Analytics:	mask |= 0x01;	break;
			case tt__CapabilityCategory__Device:	mask |= 0x02;	break;
			case tt__CapabilityCategory__Events:	mask |= 0x04;	break;
			case tt__CapabilityCategory__Imaging:	mask |= 0x08;	break;
			case tt__CapabilityCategory__Media:		mask |= 0x10;	break;
			case tt__CapabilityCategory__PTZ:		mask |= 0x20;	break;
		}
	}
	DBG_PRINT("mask = %x", mask);
	
	tds__GetCapabilitiesResponse->Capabilities = soap_new_tt__Capabilities(soap, 1);
	
	if (mask & 0x02)
	{
		//------------
		// Device
		//------------
		
		tds__GetCapabilitiesResponse->Capabilities->Device = soap_new_tt__DeviceCapabilities(soap, 1);
		
		char addr[256];
		sprintf(addr, "http://%s:%d/onvif/device_service", ip_address, onvif_port_no);
		tds__GetCapabilitiesResponse->Capabilities->Device->XAddr = soap_strdup(soap, addr);
		
		tds__GetCapabilitiesResponse->Capabilities->Device->Network = soap_new_tt__NetworkCapabilities(soap, 1);
		tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPFilter = xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Network->ZeroConfiguration = xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPVersion6 = xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->Network->DynDNS = xsd__boolean__false_;
	
		tds__GetCapabilitiesResponse->Capabilities->Device->System = soap_new_tt__SystemCapabilities(soap, 1);
		tds__GetCapabilitiesResponse->Capabilities->Device->System->DiscoveryResolve = xsd__boolean__true_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->DiscoveryBye = xsd__boolean__true_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->RemoteDiscovery = xsd__boolean__false_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SystemBackup = xsd__boolean__true_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SystemLogging = xsd__boolean__true_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->FirmwareUpgrade = xsd__boolean__true_;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->__sizeSupportedVersions = 1;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions = soap_new_tt__OnvifVersion(soap, 1);
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions->Major = 2;
		tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions->Minor = 0;
	}
	
	if (mask & 0x04)
	{
		//------------
		// Events
		//------------
		
		tds__GetCapabilitiesResponse->Capabilities->Events = soap_new_tt__EventCapabilities(soap, 1);
		char addr[256];
		sprintf(addr, "http://%s:%d/onvif/event", ip_address, onvif_port_no);
		tds__GetCapabilitiesResponse->Capabilities->Events->XAddr = soap_strdup(soap, addr);
		tds__GetCapabilitiesResponse->Capabilities->Events->WSSubscriptionPolicySupport                   = xsd__boolean__true_;
		tds__GetCapabilitiesResponse->Capabilities->Events->WSPullPointSupport                            = xsd__boolean__true_;
		tds__GetCapabilitiesResponse->Capabilities->Events->WSPausableSubscriptionManagerInterfaceSupport = xsd__boolean__false_;
	}
	
	if (mask & 0x10)
	{
		//------------
		// Media
		//------------
		
		tds__GetCapabilitiesResponse->Capabilities->Media = soap_new_tt__MediaCapabilities(soap, 1);
		char addr[256];
		sprintf(addr, "http://%s:%d/onvif/media", ip_address, onvif_port_no);
		tds__GetCapabilitiesResponse->Capabilities->Media->XAddr = soap_strdup(soap, addr);
		tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities = soap_new_tt__RealTimeStreamingCapabilities(soap, 1);
	}
	
	if (mask & 0x20)
	{
		//------------
		// PTZ
		//------------
		
		if (Bridge_IsSupportedPTZ() == 1)
		{
			tds__GetCapabilitiesResponse->Capabilities->PTZ = soap_new_tt__PTZCapabilities(soap, 1);
			char addr[256];
			sprintf(addr, "http://%s:%d/onvif/ptz", ip_address, onvif_port_no);
			tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr = soap_strdup(soap, addr);
		}
	}

	return 0;
}

/** Web service operation '__tds__SetDPAddresses' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDPAddresses(struct soap* soap, struct _tds__SetDPAddresses *tds__SetDPAddresses, struct _tds__SetDPAddressesResponse *tds__SetDPAddressesResponse)
{
	DBG_TRACE("### __tds__SetDPAddresses ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetHostname' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetHostname(struct soap* soap, struct _tds__GetHostname *tds__GetHostname, struct _tds__GetHostnameResponse *tds__GetHostnameResponse)
{
	DBG_TRACE("### __tds__GetHostname ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	char hostname[80];
	Bridge_GetHostname(hostname);
	
	tds__GetHostnameResponse->HostnameInformation = soap_new_tt__HostnameInformation(soap, 1);
	tds__GetHostnameResponse->HostnameInformation->Name = soap_strdup(soap, hostname);
	tds__GetHostnameResponse->HostnameInformation->FromDHCP = xsd__boolean__false_;
	
	return 0;
}

/** Web service operation '__tds__SetHostname' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHostname(struct soap* soap, struct _tds__SetHostname *tds__SetHostname, struct _tds__SetHostnameResponse *tds__SetHostnameResponse)
{
	DBG_TRACE("### __tds__SetHostname ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("Name = %s", tds__SetHostname->Name);
	Bridge_SetHostname(tds__SetHostname->Name);
	
	return 0;
}

/** Web service operation '__tds__SetHostnameFromDHCP' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHostnameFromDHCP(struct soap* soap, struct _tds__SetHostnameFromDHCP *tds__SetHostnameFromDHCP, struct _tds__SetHostnameFromDHCPResponse *tds__SetHostnameFromDHCPResponse)
{
	DBG_TRACE("### __tds__SetHostnameFromDHCP ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDNS' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDNS(struct soap* soap, struct _tds__GetDNS *tds__GetDNS, struct _tds__GetDNSResponse *tds__GetDNSResponse)
{
	DBG_TRACE("### __tds__GetDNS ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	int use_dhcp;
	char dns_address[80];
	Bridge_GetDnsAddress(&use_dhcp, dns_address);
	
	tds__GetDNSResponse->DNSInformation = soap_new_tt__DNSInformation(soap, 1);
	
	if (use_dhcp == 1)
	{
		tds__GetDNSResponse->DNSInformation->FromDHCP = xsd__boolean__true_;
		
		if(strlen(dns_address) > 0)
		{
			tds__GetDNSResponse->DNSInformation->__sizeDNSFromDHCP = 1;
			
			tds__GetDNSResponse->DNSInformation->DNSFromDHCP = soap_new_tt__IPAddress(soap, 1);
			tds__GetDNSResponse->DNSInformation->DNSFromDHCP->Type        = tt__IPType__IPv4;
			tds__GetDNSResponse->DNSInformation->DNSFromDHCP->IPv4Address = strdup(dns_address);
		}
	}
	else
	{
		tds__GetDNSResponse->DNSInformation->FromDHCP = xsd__boolean__false_;

		if(strlen(dns_address) > 0)
		{
			tds__GetDNSResponse->DNSInformation->__sizeDNSManual = 1;
			
			tds__GetDNSResponse->DNSInformation->DNSManual = soap_new_tt__IPAddress(soap, 1);
			tds__GetDNSResponse->DNSInformation->DNSManual->Type        = tt__IPType__IPv4;
			tds__GetDNSResponse->DNSInformation->DNSManual->IPv4Address = strdup(dns_address);
		}
	}
	
	return 0;
}

/** Web service operation '__tds__SetDNS' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDNS(struct soap* soap, struct _tds__SetDNS *tds__SetDNS, struct _tds__SetDNSResponse *tds__SetDNSResponse)
{
	DBG_TRACE("### __tds__SetDNS ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("FromDHCP = %d", tds__SetDNS->FromDHCP);

	if (tds__SetDNS->__sizeDNSManual == 0)
	{
		Bridge_SetDnsAddress(tds__SetDNS->FromDHCP, "");
	}
	else
	{
		if (tds__SetDNS->DNSManual->Type != tt__IPType__IPv4)
		{
			return soap_receiver_fault(soap, "could not set DNS", soap_strdup(soap, "IPv6 not support"));
		}
		
		Bridge_SetDnsAddress(tds__SetDNS->FromDHCP, tds__SetDNS->DNSManual->IPv4Address);
	}

	return 0;
}

/** Web service operation '__tds__GetNTP' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNTP(struct soap* soap, struct _tds__GetNTP *tds__GetNTP, struct _tds__GetNTPResponse *tds__GetNTPResponse)
{
	DBG_TRACE("### __tds__GetNTP ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	tds__GetNTPResponse->NTPInformation = soap_new_tt__NTPInformation(soap, 1);
	tds__GetNTPResponse->NTPInformation->FromDHCP = xsd__boolean__false_;
	tds__GetNTPResponse->NTPInformation->__sizeNTPManual = 0;
	
	return 0;
}

/** Web service operation '__tds__SetNTP' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNTP(struct soap* soap, struct _tds__SetNTP *tds__SetNTP, struct _tds__SetNTPResponse *tds__SetNTPResponse)
{
	DBG_TRACE("### __tds__SetNTP ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDynamicDNS' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDynamicDNS(struct soap* soap, struct _tds__GetDynamicDNS *tds__GetDynamicDNS, struct _tds__GetDynamicDNSResponse *tds__GetDynamicDNSResponse)
{
	DBG_TRACE("### __tds__GetDynamicDNS ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetDynamicDNS' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDynamicDNS(struct soap* soap, struct _tds__SetDynamicDNS *tds__SetDynamicDNS, struct _tds__SetDynamicDNSResponse *tds__SetDynamicDNSResponse)
{
	DBG_TRACE("### __tds__SetDynamicDNS ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetNetworkInterfaces' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkInterfaces(struct soap* soap, struct _tds__GetNetworkInterfaces *tds__GetNetworkInterfaces, struct _tds__GetNetworkInterfacesResponse *tds__GetNetworkInterfacesResponse)
{
	DBG_TRACE("### __tds__GetNetworkInterfaces ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	char if_name[20];
	char hw_address[20];
	int use_dhcp;
	char ip_address[256];
	int prefix_len;
	Bridge_GetNetwork(if_name, hw_address, &use_dhcp, ip_address, &prefix_len);
	
	tds__GetNetworkInterfacesResponse->__sizeNetworkInterfaces = 1;
	tds__GetNetworkInterfacesResponse->NetworkInterfaces = soap_new_tt__NetworkInterface(soap, 1);
	
	tds__GetNetworkInterfacesResponse->NetworkInterfaces->token = soap_strdup(soap, if_name);
	tds__GetNetworkInterfacesResponse->NetworkInterfaces->Enabled = xsd__boolean__true_;
	
	tds__GetNetworkInterfacesResponse->NetworkInterfaces->Info = soap_new_tt__NetworkInterfaceInfo(soap, 1);
	tds__GetNetworkInterfacesResponse->NetworkInterfaces->Info->Name      = soap_strdup(soap, if_name);
	tds__GetNetworkInterfacesResponse->NetworkInterfaces->Info->HwAddress = soap_strdup(soap, hw_address);

	tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4 = soap_new_tt__IPv4NetworkInterface(soap, 1);
	tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Enabled = xsd__boolean__true_;
	
	tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config = soap_new_tt__IPv4Configuration(soap, 1);

	if (use_dhcp == 1)
	{
		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->DHCP = xsd__boolean__true_;
	
		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->FromDHCP = soap_new_tt__PrefixedIPv4Address(soap, 1);
		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->FromDHCP->Address      = soap_strdup(soap, ip_address);
		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->FromDHCP->PrefixLength = prefix_len;
	}
	else
	{
		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->DHCP = xsd__boolean__false_;

		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->__sizeManual = 1;
		
		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->Manual = soap_new_tt__PrefixedIPv4Address(soap, 1);
		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->Manual->Address      = soap_strdup(soap, ip_address);
		tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->Manual->PrefixLength = prefix_len;
	}
	
	return 0;
}

/** Web service operation '__tds__SetNetworkInterfaces' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkInterfaces(struct soap* soap, struct _tds__SetNetworkInterfaces *tds__SetNetworkInterfaces, struct _tds__SetNetworkInterfacesResponse *tds__SetNetworkInterfacesResponse)
{
	DBG_TRACE("### __tds__SetNetworkInterfaces ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("InterfaceToken = %s", tds__SetNetworkInterfaces->InterfaceToken);

	if (tds__SetNetworkInterfaces->NetworkInterface->Enabled != NULL)
	{
		DBG_PRINT("Enabled = %d", tds__SetNetworkInterfaces->NetworkInterface->Enabled);
	}
	
	if (tds__SetNetworkInterfaces->NetworkInterface->IPv4 == NULL)
	{
		return soap_receiver_fault(soap, "could not set network", soap_strdup(soap, "IPv6 not support"));
	}
	
	if ( tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP != NULL &&
		*tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP == xsd__boolean__true_)
	{
		Bridge_SetNetwork(
			tds__SetNetworkInterfaces->InterfaceToken,
			1,
			"",
			0
			);
	}
	else
	{
		if (tds__SetNetworkInterfaces->NetworkInterface->IPv4->__sizeManual == 0)
		{
			return soap_receiver_fault(soap, "could not set network", soap_strdup(soap, "address is empty"));
		}
		else
		{
			Bridge_SetNetwork(
				tds__SetNetworkInterfaces->InterfaceToken,
				0,
				tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->Address,
				tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->PrefixLength
				);
		}
	}
	
	return 0;
}

/** Web service operation '__tds__GetNetworkProtocols' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkProtocols(struct soap* soap, struct _tds__GetNetworkProtocols *tds__GetNetworkProtocols, struct _tds__GetNetworkProtocolsResponse *tds__GetNetworkProtocolsResponse)
{
	DBG_TRACE("### __tds__GetNetworkProtocols ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	tds__GetNetworkProtocolsResponse->NetworkProtocols = soap_new_tt__NetworkProtocol(soap, 2);
	
	tds__GetNetworkProtocolsResponse->__sizeNetworkProtocols = 2;

	tds__GetNetworkProtocolsResponse->NetworkProtocols[0].Name       = tt__NetworkProtocolType__RTSP;
	tds__GetNetworkProtocolsResponse->NetworkProtocols[0].Enabled    = xsd__boolean__true_;
	tds__GetNetworkProtocolsResponse->NetworkProtocols[0].__sizePort = 1;
	tds__GetNetworkProtocolsResponse->NetworkProtocols[0].Port       = (int *)soap_malloc(soap, sizeof(int));
	tds__GetNetworkProtocolsResponse->NetworkProtocols[0].Port[0]    = Bridge_GetRtspPortNo();
	
	tds__GetNetworkProtocolsResponse->NetworkProtocols[1].Name       = tt__NetworkProtocolType__HTTP;
	tds__GetNetworkProtocolsResponse->NetworkProtocols[1].Enabled    = xsd__boolean__true_;
	tds__GetNetworkProtocolsResponse->NetworkProtocols[1].__sizePort = 1;
	tds__GetNetworkProtocolsResponse->NetworkProtocols[1].Port       = (int *)soap_malloc(soap, sizeof(int));
	tds__GetNetworkProtocolsResponse->NetworkProtocols[1].Port[0]    = Bridge_GetSnapShotPortNo();
	
	return 0;
}

/** Web service operation '__tds__SetNetworkProtocols' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkProtocols(struct soap* soap, struct _tds__SetNetworkProtocols *tds__SetNetworkProtocols, struct _tds__SetNetworkProtocolsResponse *tds__SetNetworkProtocolsResponse)
{
	DBG_TRACE("### __tds__SetNetworkProtocols ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}

	{
		int i;
		for (i = 0; i < tds__SetNetworkProtocols->__sizeNetworkProtocols; i++)
		{
			if (tds__SetNetworkProtocols->NetworkProtocols[i].Name == tt__NetworkProtocolType__RTSP)
			{
				if (tds__SetNetworkProtocols->NetworkProtocols[i].__sizePort > 0)
				{
					Bridge_SetRtspPortNo(tds__SetNetworkProtocols->NetworkProtocols[i].Port[0]);
				}
			}
			else if (tds__SetNetworkProtocols->NetworkProtocols[i].Name == tt__NetworkProtocolType__HTTP)
			{
				if (tds__SetNetworkProtocols->NetworkProtocols[i].__sizePort > 0)
				{
					Bridge_SetSnapShotPortNo(tds__SetNetworkProtocols->NetworkProtocols[i].Port[0]);
				}
			}
		}
	}
	
	return 0;
}

/** Web service operation '__tds__GetNetworkDefaultGateway' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkDefaultGateway(struct soap* soap, struct _tds__GetNetworkDefaultGateway *tds__GetNetworkDefaultGateway, struct _tds__GetNetworkDefaultGatewayResponse *tds__GetNetworkDefaultGatewayResponse)
{
	DBG_TRACE("### __tds__GetNetworkDefaultGateway ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	char address[80];
	Bridge_GetGatewayAddress(address);
	
	tds__GetNetworkDefaultGatewayResponse->NetworkGateway = soap_new_tt__NetworkGateway(soap, 1);
	
	if (strlen(address) == 0)
	{
		tds__GetNetworkDefaultGatewayResponse->NetworkGateway->__sizeIPv4Address = 0;
	}
	else
	{
		tds__GetNetworkDefaultGatewayResponse->NetworkGateway->__sizeIPv4Address = 1;
		tds__GetNetworkDefaultGatewayResponse->NetworkGateway->IPv4Address = (char**)soap_malloc(soap, sizeof(char*));
		tds__GetNetworkDefaultGatewayResponse->NetworkGateway->IPv4Address[0] = strdup(address);
	}
	
	return 0;
}

/** Web service operation '__tds__SetNetworkDefaultGateway' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkDefaultGateway(struct soap* soap, struct _tds__SetNetworkDefaultGateway *tds__SetNetworkDefaultGateway, struct _tds__SetNetworkDefaultGatewayResponse *tds__SetNetworkDefaultGatewayResponse)
{
	DBG_TRACE("### __tds__SetNetworkDefaultGateway ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}	
	
	if (tds__SetNetworkDefaultGateway->__sizeIPv4Address == 0)
	{
		Bridge_SetGatewayAddress("");
	}
	else
	{
		Bridge_SetGatewayAddress(tds__SetNetworkDefaultGateway->IPv4Address[0]);
	}
	
	return 0;
}

/** Web service operation '__tds__GetZeroConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetZeroConfiguration(struct soap* soap, struct _tds__GetZeroConfiguration *tds__GetZeroConfiguration, struct _tds__GetZeroConfigurationResponse *tds__GetZeroConfigurationResponse)
{
	DBG_TRACE("### __tds__GetZeroConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetZeroConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetZeroConfiguration(struct soap* soap, struct _tds__SetZeroConfiguration *tds__SetZeroConfiguration, struct _tds__SetZeroConfigurationResponse *tds__SetZeroConfigurationResponse)
{
	DBG_TRACE("### __tds__SetZeroConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetIPAddressFilter' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetIPAddressFilter(struct soap* soap, struct _tds__GetIPAddressFilter *tds__GetIPAddressFilter, struct _tds__GetIPAddressFilterResponse *tds__GetIPAddressFilterResponse)
{
	DBG_TRACE("### __tds__GetIPAddressFilter ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetIPAddressFilter' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetIPAddressFilter(struct soap* soap, struct _tds__SetIPAddressFilter *tds__SetIPAddressFilter, struct _tds__SetIPAddressFilterResponse *tds__SetIPAddressFilterResponse)
{
	DBG_TRACE("### __tds__SetIPAddressFilter ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__AddIPAddressFilter' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__AddIPAddressFilter(struct soap* soap, struct _tds__AddIPAddressFilter *tds__AddIPAddressFilter, struct _tds__AddIPAddressFilterResponse *tds__AddIPAddressFilterResponse)
{
	DBG_TRACE("### __tds__AddIPAddressFilter ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__RemoveIPAddressFilter' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RemoveIPAddressFilter(struct soap* soap, struct _tds__RemoveIPAddressFilter *tds__RemoveIPAddressFilter, struct _tds__RemoveIPAddressFilterResponse *tds__RemoveIPAddressFilterResponse)
{
	DBG_TRACE("### __tds__RemoveIPAddressFilter ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetAccessPolicy' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetAccessPolicy(struct soap* soap, struct _tds__GetAccessPolicy *tds__GetAccessPolicy, struct _tds__GetAccessPolicyResponse *tds__GetAccessPolicyResponse)
{
	DBG_TRACE("### __tds__GetAccessPolicy ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetAccessPolicy' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetAccessPolicy(struct soap* soap, struct _tds__SetAccessPolicy *tds__SetAccessPolicy, struct _tds__SetAccessPolicyResponse *tds__SetAccessPolicyResponse)
{
	DBG_TRACE("### __tds__SetAccessPolicy ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__CreateCertificate' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateCertificate(struct soap* soap, struct _tds__CreateCertificate *tds__CreateCertificate, struct _tds__CreateCertificateResponse *tds__CreateCertificateResponse)
{
	DBG_TRACE("### __tds__CreateCertificate ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetCertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificates(struct soap* soap, struct _tds__GetCertificates *tds__GetCertificates, struct _tds__GetCertificatesResponse *tds__GetCertificatesResponse)
{
	DBG_TRACE("### __tds__GetCertificates ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetCertificatesStatus' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificatesStatus(struct soap* soap, struct _tds__GetCertificatesStatus *tds__GetCertificatesStatus, struct _tds__GetCertificatesStatusResponse *tds__GetCertificatesStatusResponse)
{
	DBG_TRACE("### __tds__GetCertificatesStatus ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetCertificatesStatus' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetCertificatesStatus(struct soap* soap, struct _tds__SetCertificatesStatus *tds__SetCertificatesStatus, struct _tds__SetCertificatesStatusResponse *tds__SetCertificatesStatusResponse)
{
	DBG_TRACE("### __tds__SetCertificatesStatus ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__DeleteCertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteCertificates(struct soap* soap, struct _tds__DeleteCertificates *tds__DeleteCertificates, struct _tds__DeleteCertificatesResponse *tds__DeleteCertificatesResponse)
{
	DBG_TRACE("### __tds__DeleteCertificates ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetPkcs10Request' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPkcs10Request(struct soap* soap, struct _tds__GetPkcs10Request *tds__GetPkcs10Request, struct _tds__GetPkcs10RequestResponse *tds__GetPkcs10RequestResponse)
{
	DBG_TRACE("### __tds__GetPkcs10Request ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__LoadCertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCertificates(struct soap* soap, struct _tds__LoadCertificates *tds__LoadCertificates, struct _tds__LoadCertificatesResponse *tds__LoadCertificatesResponse)
{
	DBG_TRACE("### __tds__LoadCertificates ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetClientCertificateMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetClientCertificateMode(struct soap* soap, struct _tds__GetClientCertificateMode *tds__GetClientCertificateMode, struct _tds__GetClientCertificateModeResponse *tds__GetClientCertificateModeResponse)
{
	DBG_TRACE("### __tds__GetClientCertificateMode ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetClientCertificateMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetClientCertificateMode(struct soap* soap, struct _tds__SetClientCertificateMode *tds__SetClientCertificateMode, struct _tds__SetClientCertificateModeResponse *tds__SetClientCertificateModeResponse)
{
	DBG_TRACE("### __tds__SetClientCertificateMode ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetRelayOutputs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRelayOutputs(struct soap* soap, struct _tds__GetRelayOutputs *tds__GetRelayOutputs, struct _tds__GetRelayOutputsResponse *tds__GetRelayOutputsResponse)
{
	DBG_TRACE("### __tds__GetRelayOutputs ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetRelayOutputSettings' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRelayOutputSettings(struct soap* soap, struct _tds__SetRelayOutputSettings *tds__SetRelayOutputSettings, struct _tds__SetRelayOutputSettingsResponse *tds__SetRelayOutputSettingsResponse)
{
	DBG_TRACE("### __tds__SetRelayOutputSettings ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetRelayOutputState' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRelayOutputState(struct soap* soap, struct _tds__SetRelayOutputState *tds__SetRelayOutputState, struct _tds__SetRelayOutputStateResponse *tds__SetRelayOutputStateResponse)
{
	DBG_TRACE("### __tds__SetRelayOutputState ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SendAuxiliaryCommand' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SendAuxiliaryCommand(struct soap* soap, struct _tds__SendAuxiliaryCommand *tds__SendAuxiliaryCommand, struct _tds__SendAuxiliaryCommandResponse *tds__SendAuxiliaryCommandResponse)
{
	DBG_TRACE("### __tds__SendAuxiliaryCommand ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetCACertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCACertificates(struct soap* soap, struct _tds__GetCACertificates *tds__GetCACertificates, struct _tds__GetCACertificatesResponse *tds__GetCACertificatesResponse)
{
	DBG_TRACE("### __tds__GetCACertificates ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__LoadCertificateWithPrivateKey' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCertificateWithPrivateKey(struct soap* soap, struct _tds__LoadCertificateWithPrivateKey *tds__LoadCertificateWithPrivateKey, struct _tds__LoadCertificateWithPrivateKeyResponse *tds__LoadCertificateWithPrivateKeyResponse)
{
	DBG_TRACE("### __tds__LoadCertificateWithPrivateKey ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetCertificateInformation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificateInformation(struct soap* soap, struct _tds__GetCertificateInformation *tds__GetCertificateInformation, struct _tds__GetCertificateInformationResponse *tds__GetCertificateInformationResponse)
{
	DBG_TRACE("### __tds__GetCertificateInformation ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__LoadCACertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCACertificates(struct soap* soap, struct _tds__LoadCACertificates *tds__LoadCACertificates, struct _tds__LoadCACertificatesResponse *tds__LoadCACertificatesResponse)
{
	DBG_TRACE("### __tds__LoadCACertificates ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__CreateDot1XConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateDot1XConfiguration(struct soap* soap, struct _tds__CreateDot1XConfiguration *tds__CreateDot1XConfiguration, struct _tds__CreateDot1XConfigurationResponse *tds__CreateDot1XConfigurationResponse)
{
	DBG_TRACE("### __tds__CreateDot1XConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetDot1XConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDot1XConfiguration(struct soap* soap, struct _tds__SetDot1XConfiguration *tds__SetDot1XConfiguration, struct _tds__SetDot1XConfigurationResponse *tds__SetDot1XConfigurationResponse)
{
	DBG_TRACE("### __tds__SetDot1XConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDot1XConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot1XConfiguration(struct soap* soap, struct _tds__GetDot1XConfiguration *tds__GetDot1XConfiguration, struct _tds__GetDot1XConfigurationResponse *tds__GetDot1XConfigurationResponse)
{
	DBG_TRACE("### __tds__GetDot1XConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDot1XConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot1XConfigurations(struct soap* soap, struct _tds__GetDot1XConfigurations *tds__GetDot1XConfigurations, struct _tds__GetDot1XConfigurationsResponse *tds__GetDot1XConfigurationsResponse)
{
	DBG_TRACE("### __tds__GetDot1XConfigurations ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__DeleteDot1XConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteDot1XConfiguration(struct soap* soap, struct _tds__DeleteDot1XConfiguration *tds__DeleteDot1XConfiguration, struct _tds__DeleteDot1XConfigurationResponse *tds__DeleteDot1XConfigurationResponse)
{
	DBG_TRACE("### __tds__DeleteDot1XConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDot11Capabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot11Capabilities(struct soap* soap, struct _tds__GetDot11Capabilities *tds__GetDot11Capabilities, struct _tds__GetDot11CapabilitiesResponse *tds__GetDot11CapabilitiesResponse)
{
	DBG_TRACE("### __tds__GetDot11Capabilities ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetDot11Status' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot11Status(struct soap* soap, struct _tds__GetDot11Status *tds__GetDot11Status, struct _tds__GetDot11StatusResponse *tds__GetDot11StatusResponse)
{
	DBG_TRACE("### __tds__GetDot11Status ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__ScanAvailableDot11Networks' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__ScanAvailableDot11Networks(struct soap* soap, struct _tds__ScanAvailableDot11Networks *tds__ScanAvailableDot11Networks, struct _tds__ScanAvailableDot11NetworksResponse *tds__ScanAvailableDot11NetworksResponse)
{
	DBG_TRACE("### __tds__ScanAvailableDot11Networks ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetSystemUris' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemUris(struct soap* soap, struct _tds__GetSystemUris *tds__GetSystemUris, struct _tds__GetSystemUrisResponse *tds__GetSystemUrisResponse)
{
	DBG_TRACE("### __tds__GetSystemUris ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__StartFirmwareUpgrade' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__StartFirmwareUpgrade(struct soap* soap, struct _tds__StartFirmwareUpgrade *tds__StartFirmwareUpgrade, struct _tds__StartFirmwareUpgradeResponse *tds__StartFirmwareUpgradeResponse)
{
	DBG_TRACE("### __tds__StartFirmwareUpgrade ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__StartSystemRestore' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__StartSystemRestore(struct soap* soap, struct _tds__StartSystemRestore *tds__StartSystemRestore, struct _tds__StartSystemRestoreResponse *tds__StartSystemRestoreResponse)
{
	DBG_TRACE("### __tds__StartSystemRestore ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetStorageConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetStorageConfigurations(struct soap* soap, struct _tds__GetStorageConfigurations *tds__GetStorageConfigurations, struct _tds__GetStorageConfigurationsResponse *tds__GetStorageConfigurationsResponse)
{
	DBG_TRACE("### __tds__GetStorageConfigurations ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__CreateStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateStorageConfiguration(struct soap* soap, struct _tds__CreateStorageConfiguration *tds__CreateStorageConfiguration, struct _tds__CreateStorageConfigurationResponse *tds__CreateStorageConfigurationResponse)
{
	DBG_TRACE("### __tds__CreateStorageConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetStorageConfiguration(struct soap* soap, struct _tds__GetStorageConfiguration *tds__GetStorageConfiguration, struct _tds__GetStorageConfigurationResponse *tds__GetStorageConfigurationResponse)
{
	DBG_TRACE("### __tds__GetStorageConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetStorageConfiguration(struct soap* soap, struct _tds__SetStorageConfiguration *tds__SetStorageConfiguration, struct _tds__SetStorageConfigurationResponse *tds__SetStorageConfigurationResponse)
{
	DBG_TRACE("### __tds__SetStorageConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__DeleteStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteStorageConfiguration(struct soap* soap, struct _tds__DeleteStorageConfiguration *tds__DeleteStorageConfiguration, struct _tds__DeleteStorageConfigurationResponse *tds__DeleteStorageConfigurationResponse)
{
	DBG_TRACE("### __tds__DeleteStorageConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__GetGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetGeoLocation(struct soap* soap, struct _tds__GetGeoLocation *tds__GetGeoLocation, struct _tds__GetGeoLocationResponse *tds__GetGeoLocationResponse)
{
	DBG_TRACE("### __tds__GetGeoLocation ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__SetGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetGeoLocation(struct soap* soap, struct _tds__SetGeoLocation *tds__SetGeoLocation, struct _tds__SetGeoLocationResponse *tds__SetGeoLocationResponse)
{
	DBG_TRACE("### __tds__SetGeoLocation ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tds__DeleteGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteGeoLocation(struct soap* soap, struct _tds__DeleteGeoLocation *tds__DeleteGeoLocation, struct _tds__DeleteGeoLocationResponse *tds__DeleteGeoLocationResponse)
{
	DBG_TRACE("### __tds__DeleteGeoLocation ###");
	return SOAP_NO_METHOD;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// #### MEDIA ####
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void SetVideoSourceConfiguration(struct soap* soap, struct tt__VideoSourceConfiguration* dst, struct ONVIF_VIDEO_SOURCE_CONFIG_INFO* src)
{
	dst->Name        = soap_strdup(soap, src->name);
	dst->UseCount    = src->use_count;
	dst->token       = soap_strdup(soap, src->token);
	dst->SourceToken = soap_strdup(soap, src->source_token);
	
	dst->Bounds = soap_new_tt__IntRectangle(soap, 1);
	
	dst->Bounds->x      = src->x;
	dst->Bounds->y      = src->y;
	dst->Bounds->width  = src->width;
	dst->Bounds->height = src->height;
}

static void SetVideoEncoderConfiguration(struct soap* soap, struct tt__VideoEncoderConfiguration* dst, struct ONVIF_VIDEO_ENCODER_CONFIG_INFO* src)
{
	dst->Name        = soap_strdup(soap, src->name);
	dst->UseCount    = src->use_count;
	dst->token       = soap_strdup(soap, src->token);

	dst->Encoding = tt__VideoEncoding__H264;
	
	dst->Resolution = (struct tt__VideoResolution *)soap_malloc(soap, sizeof(struct tt__VideoResolution));
	dst->Resolution->Width  = src->width;
	dst->Resolution->Height = src->height;

	dst->Quality = src->quality;
	
	dst->H264 = soap_new_tt__H264Configuration(soap, 1);
	dst->H264->GovLength   = 32;
	dst->H264->H264Profile = tt__H264Profile__High;
}

static void SetPTZConfiguration(struct soap* soap, struct tt__PTZConfiguration* dst, struct ONVIF_PTZ_CONFIG_INFO* src)
{
	dst->Name = soap_strdup(soap, src->name);
	dst->UseCount = src->use_count;
	dst->token = soap_strdup(soap, src->token);
	dst->NodeToken = soap_strdup(soap, src->node_token);
	dst->DefaultAbsolutePantTiltPositionSpace   = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
	dst->DefaultRelativePanTiltTranslationSpace = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace");
	dst->DefaultContinuousPanTiltVelocitySpace  = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace");
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetServiceCapabilities(struct soap* soap, struct _trt__GetServiceCapabilities *trt__GetServiceCapabilities, struct _trt__GetServiceCapabilitiesResponse *trt__GetServiceCapabilitiesResponse)
{
	DBG_TRACE("### __trt__GetServiceCapabilities ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSources(struct soap* soap, struct _trt__GetVideoSources *trt__GetVideoSources, struct _trt__GetVideoSourcesResponse *trt__GetVideoSourcesResponse)
{
	DBG_TRACE("### __trt__GetVideoSources ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetVideoSourcesResponse->__sizeVideoSources = Bridge_GetVideoSourceCount();
	if (trt__GetVideoSourcesResponse->__sizeVideoSources > 0)
	{
		DBG_PRINT("__sizeVideoSources = %d", trt__GetVideoSourcesResponse->__sizeVideoSources);
		
		trt__GetVideoSourcesResponse->VideoSources = soap_new_tt__VideoSource(soap, trt__GetVideoSourcesResponse->__sizeVideoSources);
		
		{
			int i;
			for(i = 0; i < trt__GetVideoSourcesResponse->__sizeVideoSources; i++)
			{
				struct ONVIF_VIDEO_SOURCE_INFO video_source_info;
				Bridge_GetVideoSource(i, &video_source_info);
				
				trt__GetVideoSourcesResponse->VideoSources->token = soap_strdup(soap, video_source_info.token);
				trt__GetVideoSourcesResponse->VideoSources->Framerate = video_source_info.framerate;
				
				trt__GetVideoSourcesResponse->VideoSources->Resolution = soap_new_tt__VideoResolution(soap, 1);
				trt__GetVideoSourcesResponse->VideoSources->Resolution->Width  = video_source_info.width;
				trt__GetVideoSourcesResponse->VideoSources->Resolution->Height = video_source_info.height;
				
				DBG_PRINT("token     = %s", trt__GetVideoSourcesResponse->VideoSources->token);
				DBG_PRINT("Framerate = %d", trt__GetVideoSourcesResponse->VideoSources->Framerate);
				DBG_PRINT("Width     = %d", trt__GetVideoSourcesResponse->VideoSources->Resolution->Width);
				DBG_PRINT("Height    = %d", trt__GetVideoSourcesResponse->VideoSources->Resolution->Height);
			}
		}
	}

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSources(struct soap* soap, struct _trt__GetAudioSources *trt__GetAudioSources, struct _trt__GetAudioSourcesResponse *trt__GetAudioSourcesResponse)
{
	DBG_TRACE("### __trt__GetAudioSources ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetAudioSourcesResponse->__sizeAudioSources = 0;
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputs(struct soap* soap, struct _trt__GetAudioOutputs *trt__GetAudioOutputs, struct _trt__GetAudioOutputsResponse *trt__GetAudioOutputsResponse)
{
	DBG_TRACE("### __trt__GetAudioOutputs ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetAudioOutputsResponse->__sizeAudioOutputs = 0;

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__CreateProfile(struct soap* soap, struct _trt__CreateProfile *trt__CreateProfile, struct _trt__CreateProfileResponse *trt__CreateProfileResponse)
{
	DBG_TRACE("### __trt__CreateProfile ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("Name  = %s", trt__CreateProfile->Name);
	
	if (trt__CreateProfile->Token != NULL)
	{
		DBG_PRINT("Token = %s", trt__CreateProfile->Token);
	}
	
	// プロファイルを作成する
	struct ONVIF_PROFILE_INFO profile_info;
	if (Bridge_CreateProfile(trt__CreateProfile->Name, trt__CreateProfile->Token, &profile_info) != 0)
	{
		return soap_receiver_fault(soap, "could not create profile", soap_strdup(soap, "reach max profile num"));
	}
	
	trt__CreateProfileResponse->Profile = soap_new_tt__Profile(soap, 1);

	trt__CreateProfileResponse->Profile->Name  = soap_strdup(soap, profile_info.name);
	trt__CreateProfileResponse->Profile->token = soap_strdup(soap, profile_info.token);
	
	struct ONVIF_VIDEO_SOURCE_CONFIG_INFO video_source_config_info;
	if (Bridge_FindVideoSourceConfig(profile_info.video_source_config_token, &video_source_config_info) == 0)
	{
		trt__CreateProfileResponse->Profile->VideoSourceConfiguration = soap_new_tt__VideoSourceConfiguration(soap, 1);
		
		SetVideoSourceConfiguration(soap, trt__CreateProfileResponse->Profile->VideoSourceConfiguration, &video_source_config_info);
	}

	struct ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
	if (Bridge_FindVideoEncoderConfig(profile_info.video_encoder_token, &video_encoder_config_info) == 0)
	{
		trt__CreateProfileResponse->Profile->VideoEncoderConfiguration = soap_new_tt__VideoEncoderConfiguration(soap, 1);

		SetVideoEncoderConfiguration(soap, trt__CreateProfileResponse->Profile->VideoEncoderConfiguration, &video_encoder_config_info);
	}
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetProfile(struct soap* soap, struct _trt__GetProfile *trt__GetProfile, struct _trt__GetProfileResponse *trt__GetProfileResponse)
{
	DBG_TRACE("### __trt__GetProfile ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s", trt__GetProfile->ProfileToken);
	
	struct ONVIF_PROFILE_INFO profile_info;
	if (Bridge_FindProfile(trt__GetProfile->ProfileToken, &profile_info) != 0)
	{
		return SOAP_ERR;
	}

	trt__GetProfileResponse->Profile = soap_new_tt__Profile(soap, 1);

	trt__GetProfileResponse->Profile->Name  = soap_strdup(soap, profile_info.name);
	trt__GetProfileResponse->Profile->token = soap_strdup(soap, profile_info.token);
	trt__GetProfileResponse->Profile->fixed = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
	trt__GetProfileResponse->Profile->fixed[0] = xsd__boolean__true_;
		
	struct ONVIF_VIDEO_SOURCE_CONFIG_INFO video_source_config_info;
	if (Bridge_FindVideoSourceConfig(profile_info.video_source_config_token, &video_source_config_info) == 0)
	{
		trt__GetProfileResponse->Profile->VideoSourceConfiguration = soap_new_tt__VideoSourceConfiguration(soap, 1);
		SetVideoSourceConfiguration(soap, trt__GetProfileResponse->Profile->VideoSourceConfiguration, &video_source_config_info);
	}

	struct ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
	if (Bridge_FindVideoEncoderConfig(profile_info.video_encoder_token, &video_encoder_config_info) == 0)
	{
		trt__GetProfileResponse->Profile->VideoEncoderConfiguration = soap_new_tt__VideoEncoderConfiguration(soap, 1);
		SetVideoEncoderConfiguration(soap, trt__GetProfileResponse->Profile->VideoEncoderConfiguration, &video_encoder_config_info);
	}

	struct ONVIF_PTZ_CONFIG_INFO ptz_config_info;
	if (Bridge_FindPtzConfig(profile_info.ptz_token, &ptz_config_info) == 0)
	{
		trt__GetProfileResponse->Profile->PTZConfiguration = soap_new_tt__PTZConfiguration(soap, 1);
		SetPTZConfiguration(soap, trt__GetProfileResponse->Profile->PTZConfiguration, &ptz_config_info);
	}
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetProfiles(struct soap* soap, struct _trt__GetProfiles *trt__GetProfiles, struct _trt__GetProfilesResponse *trt__GetProfilesResponse)
{
	DBG_TRACE("### __trt__GetProfiles ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetProfilesResponse->__sizeProfiles = Bridge_GetProfileCount();
	if (trt__GetProfilesResponse->__sizeProfiles > 0)
	{
		DBG_PRINT("__sizeProfiles = %d", trt__GetProfilesResponse->__sizeProfiles);
		
		trt__GetProfilesResponse->Profiles = soap_new_tt__Profile(soap, trt__GetProfilesResponse->__sizeProfiles);
		
		int i;
		for(i = 0; i < trt__GetProfilesResponse->__sizeProfiles; i++)
		{
			struct ONVIF_PROFILE_INFO profile_info;
			Bridge_GetProfile(i, &profile_info);

			trt__GetProfilesResponse->Profiles[i].Name  = soap_strdup(soap, profile_info.name);
			trt__GetProfilesResponse->Profiles[i].token = soap_strdup(soap, profile_info.token);
			trt__GetProfilesResponse->Profiles[i].fixed = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
			trt__GetProfilesResponse->Profiles[i].fixed[0] = xsd__boolean__true_;
			
			DBG_PRINT("Profile: Name  = %s", trt__GetProfilesResponse->Profiles[i].Name);
			DBG_PRINT("Profile: token = %s", trt__GetProfilesResponse->Profiles[i].token);
			
			struct ONVIF_VIDEO_SOURCE_CONFIG_INFO video_source_config_info;
			if (Bridge_FindVideoSourceConfig(profile_info.video_source_config_token, &video_source_config_info) == 0)
			{
				trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration = soap_new_tt__VideoSourceConfiguration(soap, 1);
				SetVideoSourceConfiguration(soap, trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration, &video_source_config_info);
			}
	
			struct ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
			if (Bridge_FindVideoEncoderConfig(profile_info.video_encoder_token, &video_encoder_config_info) == 0)
			{
				trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration = soap_new_tt__VideoEncoderConfiguration(soap, 1);
				SetVideoEncoderConfiguration(soap, trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration, &video_encoder_config_info);
			}
			
			struct ONVIF_PTZ_CONFIG_INFO ptz_config_info;
			if (Bridge_FindPtzConfig(profile_info.ptz_token, &ptz_config_info) == 0)
			{
				trt__GetProfilesResponse->Profiles[i].PTZConfiguration = soap_new_tt__PTZConfiguration(soap, 1);
				SetPTZConfiguration(soap, trt__GetProfilesResponse->Profiles[i].PTZConfiguration, &ptz_config_info);
			}
		}
	}
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoEncoderConfiguration(struct soap* soap, struct _trt__AddVideoEncoderConfiguration *trt__AddVideoEncoderConfiguration, struct _trt__AddVideoEncoderConfigurationResponse *trt__AddVideoEncoderConfigurationResponse)
{
	DBG_TRACE("### __trt__AddVideoEncoderConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken       = %s", trt__AddVideoEncoderConfiguration->ProfileToken);
	DBG_PRINT("ConfigurationToken = %s", trt__AddVideoEncoderConfiguration->ConfigurationToken);

	Bridge_AddVideoEncoder(trt__AddVideoEncoderConfiguration->ProfileToken, trt__AddVideoEncoderConfiguration->ConfigurationToken);
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoSourceConfiguration(struct soap* soap, struct _trt__AddVideoSourceConfiguration *trt__AddVideoSourceConfiguration, struct _trt__AddVideoSourceConfigurationResponse *trt__AddVideoSourceConfigurationResponse)
{
	DBG_TRACE("### __trt__AddVideoSourceConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken       = %s", trt__AddVideoSourceConfiguration->ProfileToken);
	DBG_PRINT("ConfigurationToken = %s", trt__AddVideoSourceConfiguration->ConfigurationToken);
	
	Bridge_AddVideoSource(trt__AddVideoSourceConfiguration->ProfileToken, trt__AddVideoSourceConfiguration->ConfigurationToken);
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioEncoderConfiguration(struct soap* soap, struct _trt__AddAudioEncoderConfiguration *trt__AddAudioEncoderConfiguration, struct _trt__AddAudioEncoderConfigurationResponse *trt__AddAudioEncoderConfigurationResponse)
{
	DBG_TRACE("### __trt__AddAudioEncoderConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioSourceConfiguration(struct soap* soap, struct _trt__AddAudioSourceConfiguration *trt__AddAudioSourceConfiguration, struct _trt__AddAudioSourceConfigurationResponse *trt__AddAudioSourceConfigurationResponse)
{
	DBG_TRACE("### __trt__AddAudioSourceConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddPTZConfiguration(struct soap* soap, struct _trt__AddPTZConfiguration *trt__AddPTZConfiguration, struct _trt__AddPTZConfigurationResponse *trt__AddPTZConfigurationResponse)
{
	DBG_TRACE("### __trt__AddPTZConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken       = %s", trt__AddPTZConfiguration->ProfileToken);
	DBG_PRINT("ConfigurationToken = %s", trt__AddPTZConfiguration->ConfigurationToken);
	
	Bridge_AddPtz(trt__AddPTZConfiguration->ProfileToken, trt__AddPTZConfiguration->ConfigurationToken);
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoAnalyticsConfiguration(struct soap* soap, struct _trt__AddVideoAnalyticsConfiguration *trt__AddVideoAnalyticsConfiguration, struct _trt__AddVideoAnalyticsConfigurationResponse *trt__AddVideoAnalyticsConfigurationResponse)
{
	DBG_TRACE("### __trt__AddVideoAnalyticsConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddMetadataConfiguration(struct soap* soap, struct _trt__AddMetadataConfiguration *trt__AddMetadataConfiguration, struct _trt__AddMetadataConfigurationResponse *trt__AddMetadataConfigurationResponse)
{
	DBG_TRACE("### __trt__AddMetadataConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioOutputConfiguration(struct soap* soap, struct _trt__AddAudioOutputConfiguration *trt__AddAudioOutputConfiguration, struct _trt__AddAudioOutputConfigurationResponse *trt__AddAudioOutputConfigurationResponse)
{
	DBG_TRACE("### __trt__AddAudioOutputConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioDecoderConfiguration(struct soap* soap, struct _trt__AddAudioDecoderConfiguration *trt__AddAudioDecoderConfiguration, struct _trt__AddAudioDecoderConfigurationResponse *trt__AddAudioDecoderConfigurationResponse)
{
	DBG_TRACE("### __trt__AddAudioDecoderConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoEncoderConfiguration(struct soap* soap, struct _trt__RemoveVideoEncoderConfiguration *trt__RemoveVideoEncoderConfiguration, struct _trt__RemoveVideoEncoderConfigurationResponse *trt__RemoveVideoEncoderConfigurationResponse)
{
	DBG_TRACE("### __trt__RemoveVideoEncoderConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s", trt__RemoveVideoEncoderConfiguration->ProfileToken);
	
	Bridge_RemoveVideoEncoder(trt__RemoveVideoEncoderConfiguration->ProfileToken);
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoSourceConfiguration(struct soap* soap, struct _trt__RemoveVideoSourceConfiguration *trt__RemoveVideoSourceConfiguration, struct _trt__RemoveVideoSourceConfigurationResponse *trt__RemoveVideoSourceConfigurationResponse)
{
	DBG_TRACE("### __trt__RemoveVideoSourceConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s\n", trt__RemoveVideoSourceConfiguration->ProfileToken);
	
	Bridge_RemoveVideoSource(trt__RemoveVideoSourceConfiguration->ProfileToken);
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioEncoderConfiguration(struct soap* soap, struct _trt__RemoveAudioEncoderConfiguration *trt__RemoveAudioEncoderConfiguration, struct _trt__RemoveAudioEncoderConfigurationResponse *trt__RemoveAudioEncoderConfigurationResponse)
{
	DBG_TRACE("### __trt__RemoveAudioEncoderConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioSourceConfiguration(struct soap* soap, struct _trt__RemoveAudioSourceConfiguration *trt__RemoveAudioSourceConfiguration, struct _trt__RemoveAudioSourceConfigurationResponse *trt__RemoveAudioSourceConfigurationResponse)
{
	DBG_TRACE("### __trt__RemoveAudioSourceConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemovePTZConfiguration(struct soap* soap, struct _trt__RemovePTZConfiguration *trt__RemovePTZConfiguration, struct _trt__RemovePTZConfigurationResponse *trt__RemovePTZConfigurationResponse)
{
	DBG_TRACE("### __trt__RemovePTZConfiguration ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s", trt__RemovePTZConfiguration->ProfileToken);
	
	Bridge_RemovePtz(trt__RemovePTZConfiguration->ProfileToken);
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoAnalyticsConfiguration(struct soap* soap, struct _trt__RemoveVideoAnalyticsConfiguration *trt__RemoveVideoAnalyticsConfiguration, struct _trt__RemoveVideoAnalyticsConfigurationResponse *trt__RemoveVideoAnalyticsConfigurationResponse)
{
	DBG_TRACE("### __trt__RemoveVideoAnalyticsConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveMetadataConfiguration(struct soap* soap, struct _trt__RemoveMetadataConfiguration *trt__RemoveMetadataConfiguration, struct _trt__RemoveMetadataConfigurationResponse *trt__RemoveMetadataConfigurationResponse)
{
	DBG_TRACE("### __trt__RemoveMetadataConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioOutputConfiguration(struct soap* soap, struct _trt__RemoveAudioOutputConfiguration *trt__RemoveAudioOutputConfiguration, struct _trt__RemoveAudioOutputConfigurationResponse *trt__RemoveAudioOutputConfigurationResponse)
{
	DBG_TRACE("### __trt__RemoveAudioOutputConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioDecoderConfiguration(struct soap* soap, struct _trt__RemoveAudioDecoderConfiguration *trt__RemoveAudioDecoderConfiguration, struct _trt__RemoveAudioDecoderConfigurationResponse *trt__RemoveAudioDecoderConfigurationResponse)
{
	DBG_TRACE("### __trt__RemoveAudioDecoderConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__DeleteProfile(struct soap* soap, struct _trt__DeleteProfile *trt__DeleteProfile, struct _trt__DeleteProfileResponse *trt__DeleteProfileResponse)
{
	DBG_TRACE("### __trt__DeleteProfile ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s", trt__DeleteProfile->ProfileToken);
	
	Bridge_DeleteProfile(trt__DeleteProfile->ProfileToken);
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfigurations(struct soap* soap, struct _trt__GetVideoSourceConfigurations *trt__GetVideoSourceConfigurations, struct _trt__GetVideoSourceConfigurationsResponse *trt__GetVideoSourceConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetVideoSourceConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetVideoSourceConfigurationsResponse->__sizeConfigurations = Bridge_GetVideoSourceConfigCount();
	trt__GetVideoSourceConfigurationsResponse->Configurations = soap_new_tt__VideoSourceConfiguration(soap, trt__GetVideoSourceConfigurationsResponse->__sizeConfigurations);
	
	int i;
	for(i = 0; i < trt__GetVideoSourceConfigurationsResponse->__sizeConfigurations; i++)
	{
		struct ONVIF_VIDEO_SOURCE_CONFIG_INFO video_source_config_info;
		Bridge_GetVideoSourceConfig(i, &video_source_config_info);
		SetVideoSourceConfiguration(soap, &(trt__GetVideoSourceConfigurationsResponse->Configurations[i]), &video_source_config_info);
	}

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfigurations(struct soap* soap, struct _trt__GetVideoEncoderConfigurations *trt__GetVideoEncoderConfigurations, struct _trt__GetVideoEncoderConfigurationsResponse *trt__GetVideoEncoderConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetVideoEncoderConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	trt__GetVideoEncoderConfigurationsResponse->__sizeConfigurations = Bridge_GetVideoEncoderConfigCount();
	trt__GetVideoEncoderConfigurationsResponse->Configurations = soap_new_tt__VideoEncoderConfiguration(soap, trt__GetVideoEncoderConfigurationsResponse->__sizeConfigurations);
		
	int i = 0;
	for (i = 0; i < trt__GetVideoEncoderConfigurationsResponse->__sizeConfigurations; i++)
	{
		struct ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
		Bridge_GetVideoEncoderConfig(i, &video_encoder_config_info);
		SetVideoEncoderConfiguration(soap, &(trt__GetVideoEncoderConfigurationsResponse->Configurations[i]), &video_encoder_config_info);
	}
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfigurations(struct soap* soap, struct _trt__GetAudioSourceConfigurations *trt__GetAudioSourceConfigurations, struct _trt__GetAudioSourceConfigurationsResponse *trt__GetAudioSourceConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetAudioSourceConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetAudioSourceConfigurationsResponse->__sizeConfigurations = 0;
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfigurations(struct soap* soap, struct _trt__GetAudioEncoderConfigurations *trt__GetAudioEncoderConfigurations, struct _trt__GetAudioEncoderConfigurationsResponse *trt__GetAudioEncoderConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetAudioEncoderConfigurations ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetAudioEncoderConfigurationsResponse->__sizeConfigurations = 0;

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoAnalyticsConfigurations(struct soap* soap, struct _trt__GetVideoAnalyticsConfigurations *trt__GetVideoAnalyticsConfigurations, struct _trt__GetVideoAnalyticsConfigurationsResponse *trt__GetVideoAnalyticsConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetVideoAnalyticsConfigurations ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetVideoAnalyticsConfigurationsResponse->__sizeConfigurations = 0;

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfigurations(struct soap* soap, struct _trt__GetMetadataConfigurations *trt__GetMetadataConfigurations, struct _trt__GetMetadataConfigurationsResponse *trt__GetMetadataConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetMetadataConfigurations ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	trt__GetMetadataConfigurationsResponse->__sizeConfigurations = 0;

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfigurations(struct soap* soap, struct _trt__GetAudioOutputConfigurations *trt__GetAudioOutputConfigurations, struct _trt__GetAudioOutputConfigurationsResponse *trt__GetAudioOutputConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetAudioOutputConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetAudioOutputConfigurationsResponse->__sizeConfigurations = 0;

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfigurations(struct soap* soap, struct _trt__GetAudioDecoderConfigurations *trt__GetAudioDecoderConfigurations, struct _trt__GetAudioDecoderConfigurationsResponse *trt__GetAudioDecoderConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetAudioDecoderConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetAudioDecoderConfigurationsResponse->__sizeConfigurations = 0;

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfiguration(struct soap* soap, struct _trt__GetVideoSourceConfiguration *trt__GetVideoSourceConfiguration, struct _trt__GetVideoSourceConfigurationResponse *trt__GetVideoSourceConfigurationResponse)
{
	DBG_TRACE("### __trt__GetVideoSourceConfiguration ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ConfigurationToken = %s", trt__GetVideoSourceConfiguration->ConfigurationToken);

	struct ONVIF_VIDEO_SOURCE_CONFIG_INFO video_source_config_info;
	if (Bridge_FindVideoSourceConfig(trt__GetVideoSourceConfiguration->ConfigurationToken, &video_source_config_info) == 0)
	{
		trt__GetVideoSourceConfigurationResponse->Configuration = soap_new_tt__VideoSourceConfiguration(soap, 1);
		SetVideoSourceConfiguration(soap, trt__GetVideoSourceConfigurationResponse->Configuration, &video_source_config_info);
	}

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfiguration(struct soap* soap, struct _trt__GetVideoEncoderConfiguration *trt__GetVideoEncoderConfiguration, struct _trt__GetVideoEncoderConfigurationResponse *trt__GetVideoEncoderConfigurationResponse)
{
	DBG_TRACE("### __trt__GetVideoEncoderConfiguration ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("ConfigurationToken = %s", trt__GetVideoEncoderConfiguration->ConfigurationToken);
	
	struct ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
	if (Bridge_FindVideoEncoderConfig(trt__GetVideoEncoderConfiguration->ConfigurationToken, &video_encoder_config_info) == 0)
	{
		trt__GetVideoEncoderConfigurationResponse->Configuration = soap_new_tt__VideoEncoderConfiguration(soap, 1);
		SetVideoEncoderConfiguration(soap, trt__GetVideoEncoderConfigurationResponse->Configuration, &video_encoder_config_info);
	}
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfiguration(struct soap* soap, struct _trt__GetAudioSourceConfiguration *trt__GetAudioSourceConfiguration, struct _trt__GetAudioSourceConfigurationResponse *trt__GetAudioSourceConfigurationResponse)
{
	DBG_TRACE("### __trt__GetAudioSourceConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfiguration(struct soap* soap, struct _trt__GetAudioEncoderConfiguration *trt__GetAudioEncoderConfiguration, struct _trt__GetAudioEncoderConfigurationResponse *trt__GetAudioEncoderConfigurationResponse)
{
	DBG_TRACE("### __trt__GetAudioEncoderConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoAnalyticsConfiguration(struct soap* soap, struct _trt__GetVideoAnalyticsConfiguration *trt__GetVideoAnalyticsConfiguration, struct _trt__GetVideoAnalyticsConfigurationResponse *trt__GetVideoAnalyticsConfigurationResponse)
{
	DBG_TRACE("### __trt__GetVideoAnalyticsConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfiguration(struct soap* soap, struct _trt__GetMetadataConfiguration *trt__GetMetadataConfiguration, struct _trt__GetMetadataConfigurationResponse *trt__GetMetadataConfigurationResponse)
{
	DBG_TRACE("### __trt__GetMetadataConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfiguration(struct soap* soap, struct _trt__GetAudioOutputConfiguration *trt__GetAudioOutputConfiguration, struct _trt__GetAudioOutputConfigurationResponse *trt__GetAudioOutputConfigurationResponse)
{
	DBG_TRACE("### __trt__GetAudioOutputConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfiguration(struct soap* soap, struct _trt__GetAudioDecoderConfiguration *trt__GetAudioDecoderConfiguration, struct _trt__GetAudioDecoderConfigurationResponse *trt__GetAudioDecoderConfigurationResponse)
{
	DBG_TRACE("### __trt__GetAudioDecoderConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoEncoderConfigurations(struct soap* soap, struct _trt__GetCompatibleVideoEncoderConfigurations *trt__GetCompatibleVideoEncoderConfigurations, struct _trt__GetCompatibleVideoEncoderConfigurationsResponse *trt__GetCompatibleVideoEncoderConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetCompatibleVideoEncoderConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s", trt__GetCompatibleVideoEncoderConfigurations->ProfileToken);
	
	struct ONVIF_PROFILE_INFO profile_info;
	if (Bridge_FindProfile(trt__GetCompatibleVideoEncoderConfigurations->ProfileToken, &profile_info) != 0)
	{
		return SOAP_ERR;
	}

	int count = Bridge_GetVideoEncoderConfigCount();
	DBG_PRINT("count = %d", count);
	
	int i = 0;
	
	struct ONVIF_VIDEO_SOURCE_CONFIG_INFO video_source_config_info;
	if (Bridge_FindVideoSourceConfig(profile_info.video_source_config_token, &video_source_config_info) != 0)
	{
		trt__GetCompatibleVideoEncoderConfigurationsResponse->__sizeConfigurations = count;
		trt__GetCompatibleVideoEncoderConfigurationsResponse->Configurations = soap_new_tt__VideoEncoderConfiguration(soap, trt__GetCompatibleVideoEncoderConfigurationsResponse->__sizeConfigurations);
		
		for (i = 0; i < count; i++)
		{
			struct ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
			Bridge_GetVideoEncoderConfig(i, &video_encoder_config_info);
			SetVideoEncoderConfiguration(soap, &(trt__GetCompatibleVideoEncoderConfigurationsResponse->Configurations[i]), &video_encoder_config_info);
		}
	}
	else
	{
		int index = 0;
	
		trt__GetCompatibleVideoEncoderConfigurationsResponse->__sizeConfigurations = Bridge_GetCompatibleVideoEncoderConfigCount(
			video_source_config_info.width, 
			video_source_config_info.height
		);
		if (trt__GetCompatibleVideoEncoderConfigurationsResponse->__sizeConfigurations > 0)
		{
			trt__GetCompatibleVideoEncoderConfigurationsResponse->Configurations = soap_new_tt__VideoEncoderConfiguration(
				soap, 
				trt__GetCompatibleVideoEncoderConfigurationsResponse->__sizeConfigurations
			);
			
			for (i = 0; i < count; i++)
			{
				struct ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
				Bridge_GetVideoEncoderConfig(i, &video_encoder_config_info);
				
				if (video_source_config_info.width >= video_encoder_config_info.width && video_source_config_info.height >= video_encoder_config_info.height)
				{
					SetVideoEncoderConfiguration(soap, &(trt__GetCompatibleVideoEncoderConfigurationsResponse->Configurations[index]), &video_encoder_config_info);
					index++;
				}
			}
		}
	}
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoSourceConfigurations(struct soap* soap, struct _trt__GetCompatibleVideoSourceConfigurations *trt__GetCompatibleVideoSourceConfigurations, struct _trt__GetCompatibleVideoSourceConfigurationsResponse *trt__GetCompatibleVideoSourceConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetCompatibleVideoSourceConfigurations ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioEncoderConfigurations(struct soap* soap, struct _trt__GetCompatibleAudioEncoderConfigurations *trt__GetCompatibleAudioEncoderConfigurations, struct _trt__GetCompatibleAudioEncoderConfigurationsResponse *trt__GetCompatibleAudioEncoderConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetCompatibleAudioEncoderConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetCompatibleAudioEncoderConfigurationsResponse->__sizeConfigurations = 0;
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioSourceConfigurations(struct soap* soap, struct _trt__GetCompatibleAudioSourceConfigurations *trt__GetCompatibleAudioSourceConfigurations, struct _trt__GetCompatibleAudioSourceConfigurationsResponse *trt__GetCompatibleAudioSourceConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetCompatibleAudioSourceConfigurations ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetCompatibleAudioSourceConfigurationsResponse->__sizeConfigurations = 0;
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoAnalyticsConfigurations(struct soap* soap, struct _trt__GetCompatibleVideoAnalyticsConfigurations *trt__GetCompatibleVideoAnalyticsConfigurations, struct _trt__GetCompatibleVideoAnalyticsConfigurationsResponse *trt__GetCompatibleVideoAnalyticsConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetCompatibleVideoAnalyticsConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trt__GetCompatibleVideoAnalyticsConfigurationsResponse->__sizeConfigurations = 0;
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleMetadataConfigurations(struct soap* soap, struct _trt__GetCompatibleMetadataConfigurations *trt__GetCompatibleMetadataConfigurations, struct _trt__GetCompatibleMetadataConfigurationsResponse *trt__GetCompatibleMetadataConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetCompatibleMetadataConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	trt__GetCompatibleMetadataConfigurationsResponse->__sizeConfigurations = 0;

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioOutputConfigurations(struct soap* soap, struct _trt__GetCompatibleAudioOutputConfigurations *trt__GetCompatibleAudioOutputConfigurations, struct _trt__GetCompatibleAudioOutputConfigurationsResponse *trt__GetCompatibleAudioOutputConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetCompatibleAudioOutputConfigurations ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	trt__GetCompatibleAudioOutputConfigurationsResponse->__sizeConfigurations = 0;
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioDecoderConfigurations(struct soap* soap, struct _trt__GetCompatibleAudioDecoderConfigurations *trt__GetCompatibleAudioDecoderConfigurations, struct _trt__GetCompatibleAudioDecoderConfigurationsResponse *trt__GetCompatibleAudioDecoderConfigurationsResponse)
{
	DBG_TRACE("### __trt__GetCompatibleAudioDecoderConfigurations ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	trt__GetCompatibleAudioDecoderConfigurationsResponse->__sizeConfigurations = 0;
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoSourceConfiguration(struct soap* soap, struct _trt__SetVideoSourceConfiguration *trt__SetVideoSourceConfiguration, struct _trt__SetVideoSourceConfigurationResponse *trt__SetVideoSourceConfigurationResponse)
{
	DBG_TRACE("### __trt__SetVideoSourceConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoEncoderConfiguration(struct soap* soap, struct _trt__SetVideoEncoderConfiguration *trt__SetVideoEncoderConfiguration, struct _trt__SetVideoEncoderConfigurationResponse *trt__SetVideoEncoderConfigurationResponse)
{
	DBG_TRACE("### __trt__SetVideoEncoderConfiguration ###");

	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("Name    = %s", trt__SetVideoEncoderConfiguration->Configuration->Name);
	DBG_PRINT("token   = %s", trt__SetVideoEncoderConfiguration->Configuration->token);
	DBG_PRINT("quality = %d", trt__SetVideoEncoderConfiguration->Configuration->Quality);
	
	Bridge_ChangeVideoEncoderConfig(
		trt__SetVideoEncoderConfiguration->Configuration->token,
		trt__SetVideoEncoderConfiguration->Configuration->Quality
	);
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioSourceConfiguration(struct soap* soap, struct _trt__SetAudioSourceConfiguration *trt__SetAudioSourceConfiguration, struct _trt__SetAudioSourceConfigurationResponse *trt__SetAudioSourceConfigurationResponse)
{
	DBG_TRACE("### __trt__SetAudioSourceConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioEncoderConfiguration(struct soap* soap, struct _trt__SetAudioEncoderConfiguration *trt__SetAudioEncoderConfiguration, struct _trt__SetAudioEncoderConfigurationResponse *trt__SetAudioEncoderConfigurationResponse)
{
	DBG_TRACE("### __trt__SetAudioEncoderConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoAnalyticsConfiguration(struct soap* soap, struct _trt__SetVideoAnalyticsConfiguration *trt__SetVideoAnalyticsConfiguration, struct _trt__SetVideoAnalyticsConfigurationResponse *trt__SetVideoAnalyticsConfigurationResponse)
{
	DBG_TRACE("### __trt__SetVideoAnalyticsConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetMetadataConfiguration(struct soap* soap, struct _trt__SetMetadataConfiguration *trt__SetMetadataConfiguration, struct _trt__SetMetadataConfigurationResponse *trt__SetMetadataConfigurationResponse)
{
	DBG_TRACE("### __trt__SetMetadataConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioOutputConfiguration(struct soap* soap, struct _trt__SetAudioOutputConfiguration *trt__SetAudioOutputConfiguration, struct _trt__SetAudioOutputConfigurationResponse *trt__SetAudioOutputConfigurationResponse)
{
	DBG_TRACE("### __trt__SetAudioOutputConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioDecoderConfiguration(struct soap* soap, struct _trt__SetAudioDecoderConfiguration *trt__SetAudioDecoderConfiguration, struct _trt__SetAudioDecoderConfigurationResponse *trt__SetAudioDecoderConfigurationResponse)
{
	DBG_TRACE("### __trt__SetAudioDecoderConfiguration ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfigurationOptions(struct soap* soap, struct _trt__GetVideoSourceConfigurationOptions *trt__GetVideoSourceConfigurationOptions, struct _trt__GetVideoSourceConfigurationOptionsResponse *trt__GetVideoSourceConfigurationOptionsResponse)
{
	DBG_TRACE("### __trt__GetVideoSourceConfigurationOptions ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfigurationOptions(struct soap* soap, struct _trt__GetVideoEncoderConfigurationOptions *trt__GetVideoEncoderConfigurationOptions, struct _trt__GetVideoEncoderConfigurationOptionsResponse *trt__GetVideoEncoderConfigurationOptionsResponse)
{
	DBG_TRACE("### __trt__GetVideoEncoderConfigurationOptions ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ConfigurationToken = %s", trt__GetVideoEncoderConfigurationOptions->ConfigurationToken);
	DBG_PRINT("ProfileToken       = %s", trt__GetVideoEncoderConfigurationOptions->ProfileToken);
	
	trt__GetVideoEncoderConfigurationOptionsResponse->Options = soap_new_tt__VideoEncoderConfigurationOptions(soap, 1);
	
	trt__GetVideoEncoderConfigurationOptionsResponse->Options->QualityRange = soap_new_tt__IntRange(soap, 1);
	trt__GetVideoEncoderConfigurationOptionsResponse->Options->QualityRange->Min = 0;
	trt__GetVideoEncoderConfigurationOptionsResponse->Options->QualityRange->Max = Bridge_GetSupportedResolutionCount() - 1;
	
	trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264 = soap_new_tt__H264Options(soap, 1);

	int count = Bridge_GetSupportedResolutionCount();

	trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->__sizeResolutionsAvailable = count;
	trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->ResolutionsAvailable = soap_new_tt__VideoResolution(soap, count);
	
	int i;
	for(i = 0; i < count; i++)
	{
		int width;
		int height;
		Bridge_GetSupportedResolution(i, &width, &height);
		trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->ResolutionsAvailable[i].Width  = width;
		trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->ResolutionsAvailable[i].Height = height;
	}
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfigurationOptions(struct soap* soap, struct _trt__GetAudioSourceConfigurationOptions *trt__GetAudioSourceConfigurationOptions, struct _trt__GetAudioSourceConfigurationOptionsResponse *trt__GetAudioSourceConfigurationOptionsResponse)
{
	DBG_TRACE("### __trt__GetAudioSourceConfigurationOptions ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfigurationOptions(struct soap* soap, struct _trt__GetAudioEncoderConfigurationOptions *trt__GetAudioEncoderConfigurationOptions, struct _trt__GetAudioEncoderConfigurationOptionsResponse *trt__GetAudioEncoderConfigurationOptionsResponse)
{
	DBG_TRACE("### __trt__GetAudioEncoderConfigurationOptions ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfigurationOptions(struct soap* soap, struct _trt__GetMetadataConfigurationOptions *trt__GetMetadataConfigurationOptions, struct _trt__GetMetadataConfigurationOptionsResponse *trt__GetMetadataConfigurationOptionsResponse)
{
	DBG_TRACE("### __trt__GetMetadataConfigurationOptions ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfigurationOptions(struct soap* soap, struct _trt__GetAudioOutputConfigurationOptions *trt__GetAudioOutputConfigurationOptions, struct _trt__GetAudioOutputConfigurationOptionsResponse *trt__GetAudioOutputConfigurationOptionsResponse)
{
	DBG_TRACE("### __trt__GetAudioOutputConfigurationOptions ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfigurationOptions(struct soap* soap, struct _trt__GetAudioDecoderConfigurationOptions *trt__GetAudioDecoderConfigurationOptions, struct _trt__GetAudioDecoderConfigurationOptionsResponse *trt__GetAudioDecoderConfigurationOptionsResponse)
{
	DBG_TRACE("### __trt__GetAudioDecoderConfigurationOptions ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetGuaranteedNumberOfVideoEncoderInstances(struct soap* soap, struct _trt__GetGuaranteedNumberOfVideoEncoderInstances *trt__GetGuaranteedNumberOfVideoEncoderInstances, struct _trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse *trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse)
{
	DBG_TRACE("### __trt__GetGuaranteedNumberOfVideoEncoderInstances ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->TotalNumber = 1;
	trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->H264 = soap_malloc(soap, sizeof(int));
	*(trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse->H264) = 1;

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetStreamUri(struct soap* soap, struct _trt__GetStreamUri *trt__GetStreamUri, struct _trt__GetStreamUriResponse *trt__GetStreamUriResponse)
{
	DBG_TRACE("### __trt__GetStreamUri ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("Stream       = %d", trt__GetStreamUri->StreamSetup->Stream);
	DBG_PRINT("Protocol     = %d", trt__GetStreamUri->StreamSetup->Transport->Protocol);
	DBG_PRINT("Tunnel       = %p", trt__GetStreamUri->StreamSetup->Transport->Tunnel);
	DBG_PRINT("ProfileToken = %s", trt__GetStreamUri->ProfileToken);

	char uri[256];
	if (Bridge_GetRtspUri(trt__GetStreamUri->ProfileToken, uri) != 0)
	{
		DBG_PRINT("Error: Bridge_GetRtspUri");
		return SOAP_ERR;
	}
	DBG_PRINT("uri = %s", uri);
	
	trt__GetStreamUriResponse->MediaUri = (struct tt__MediaUri *)soap_malloc(soap, sizeof(struct tt__MediaUri));
	soap_default_tt__MediaUri(soap, trt__GetStreamUriResponse->MediaUri);

	trt__GetStreamUriResponse->MediaUri->Uri = soap_strdup(soap, uri);
	trt__GetStreamUriResponse->MediaUri->InvalidAfterConnect = xsd__boolean__false_;
	trt__GetStreamUriResponse->MediaUri->InvalidAfterReboot  = xsd__boolean__false_;
	// trt__GetStreamUriResponse->MediaUri->Timeout = soap_strdup(soap, "PT0S");

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__StartMulticastStreaming(struct soap* soap, struct _trt__StartMulticastStreaming *trt__StartMulticastStreaming, struct _trt__StartMulticastStreamingResponse *trt__StartMulticastStreamingResponse)
{
	DBG_TRACE("### __trt__StartMulticastStreaming ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__StopMulticastStreaming(struct soap* soap, struct _trt__StopMulticastStreaming *trt__StopMulticastStreaming, struct _trt__StopMulticastStreamingResponse *trt__StopMulticastStreamingResponse)
{
	DBG_TRACE("### __trt__StopMulticastStreaming ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetSynchronizationPoint(struct soap* soap, struct _trt__SetSynchronizationPoint *trt__SetSynchronizationPoint, struct _trt__SetSynchronizationPointResponse *trt__SetSynchronizationPointResponse)
{
	DBG_TRACE("### __trt__SetSynchronizationPoint ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetSnapshotUri(struct soap* soap, struct _trt__GetSnapshotUri *trt__GetSnapshotUri, struct _trt__GetSnapshotUriResponse *trt__GetSnapshotUriResponse)
{
	DBG_TRACE("### __trt__GetSnapshotUri ###");

	DBG_PRINT("ProfileToken = %s", trt__GetSnapshotUri->ProfileToken);

	char snap_shot_uri[256];
	Bridge_GetSnapShotUri(trt__GetSnapshotUri->ProfileToken, snap_shot_uri);
	DBG_PRINT("snap_shot_uri = %s", snap_shot_uri);

	trt__GetSnapshotUriResponse->MediaUri = (struct tt__MediaUri *)soap_malloc(soap, sizeof(struct tt__MediaUri));
	soap_default_tt__MediaUri(soap, trt__GetSnapshotUriResponse->MediaUri);

	trt__GetSnapshotUriResponse->MediaUri->Uri = soap_strdup(soap, snap_shot_uri);
	
	trt__GetSnapshotUriResponse->MediaUri->InvalidAfterConnect = xsd__boolean__false_;
	trt__GetSnapshotUriResponse->MediaUri->InvalidAfterReboot  = xsd__boolean__false_;
	// trt__GetSnapshotUriResponse->MediaUri->Timeout = soap_strdup(soap, "PT0S");

	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceModes(struct soap* soap, struct _trt__GetVideoSourceModes *trt__GetVideoSourceModes, struct _trt__GetVideoSourceModesResponse *trt__GetVideoSourceModesResponse)
{
	DBG_TRACE("### __trt__GetVideoSourceModes ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoSourceMode(struct soap* soap, struct _trt__SetVideoSourceMode *trt__SetVideoSourceMode, struct _trt__SetVideoSourceModeResponse *trt__SetVideoSourceModeResponse)
{
	DBG_TRACE("### __trt__SetVideoSourceMode ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSDs(struct soap* soap, struct _trt__GetOSDs *trt__GetOSDs, struct _trt__GetOSDsResponse *trt__GetOSDsResponse)
{
	DBG_TRACE("### __trt__GetOSDs ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSD(struct soap* soap, struct _trt__GetOSD *trt__GetOSD, struct _trt__GetOSDResponse *trt__GetOSDResponse)
{
	DBG_TRACE("### __trt__GetOSD ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSDOptions(struct soap* soap, struct _trt__GetOSDOptions *trt__GetOSDOptions, struct _trt__GetOSDOptionsResponse *trt__GetOSDOptionsResponse)
{
	DBG_TRACE("### __trt__GetOSDOptions ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__SetOSD(struct soap* soap, struct _trt__SetOSD *trt__SetOSD, struct _trt__SetOSDResponse *trt__SetOSDResponse)
{
	DBG_TRACE("### __trt__SetOSD ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__CreateOSD(struct soap* soap, struct _trt__CreateOSD *trt__CreateOSD, struct _trt__CreateOSDResponse *trt__CreateOSDResponse)
{
	DBG_TRACE("### __trt__CreateOSD ###");
	return SOAP_NO_METHOD;
}

SOAP_FMAC5 int SOAP_FMAC6 __trt__DeleteOSD(struct soap* soap, struct _trt__DeleteOSD *trt__DeleteOSD, struct _trt__DeleteOSDResponse *trt__DeleteOSDResponse)
{
	DBG_TRACE("### __trt__DeleteOSD ###");
	return SOAP_NO_METHOD;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// #### EVENT ####
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

SOAP_FMAC5 int SOAP_FMAC6 __tev__GetMessages(struct soap* soap, struct _wsnt__GetMessages *wsnt__GetMessages, struct _wsnt__GetMessagesResponse *wsnt__GetMessagesResponse)
{
	DBG_TRACE("### __tev__GetMessages ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__CreatePullPoint(struct soap* soap, struct _wsnt__CreatePullPoint *wsnt__CreatePullPoint, struct _wsnt__CreatePullPointResponse *wsnt__CreatePullPointResponse)
{
	DBG_TRACE("### __tev__CreatePullPoint ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__GetEventProperties(struct soap* soap, struct _tev__GetEventProperties *tev__GetEventProperties, struct _tev__GetEventPropertiesResponse *tev__GetEventPropertiesResponse)
{
	DBG_TRACE("### __tev__GetEventProperties ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Subscribe(struct soap* soap, struct _wsnt__Subscribe *wsnt__Subscribe, struct _wsnt__SubscribeResponse *wsnt__SubscribeResponse)
{
	DBG_TRACE("### __tev__Subscribe ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Renew_(struct soap* soap, struct _wsnt__Renew *wsnt__Renew, struct _wsnt__RenewResponse *wsnt__RenewResponse)
{
	DBG_TRACE("### __tev__Renew_ ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__SetSynchronizationPoint(struct soap* soap, struct _tev__SetSynchronizationPoint *tev__SetSynchronizationPoint, struct _tev__SetSynchronizationPointResponse *tev__SetSynchronizationPointResponse)
{
	DBG_TRACE("### __tev__SetSynchronizationPoint ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__CreatePullPointSubscription(struct soap* soap, struct _tev__CreatePullPointSubscription *tev__CreatePullPointSubscription, struct _tev__CreatePullPointSubscriptionResponse *tev__CreatePullPointSubscriptionResponse)
{
	DBG_TRACE("### __tev__CreatePullPointSubscription ###");

	if (soap_wsa_check(soap))
	{
    	return soap->error;
    }
    
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
    DBG_PRINT("InitialTerminationTime = %s\n", tev__CreatePullPointSubscription->InitialTerminationTime);
    
    char pull_point[256];
	Bridge_CreateMotionEvent(pull_point);
    
    char path[256];
    sprintf(path, "http://%s:9000/onvif/pullpoint/%s", Bridge_GetIpAddres(), pull_point);
    
	tev__CreatePullPointSubscriptionResponse->SubscriptionReference.Address = soap_strdup(soap, path);
	
	tev__CreatePullPointSubscriptionResponse->wsnt__CurrentTime     = time(NULL);
	tev__CreatePullPointSubscriptionResponse->wsnt__TerminationTime = time(NULL);

	const char *ResponseMessageID = soap_wsa_rand_uuid(soap);
	const char *ResponseAction = "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionResponse";

	return soap_wsa_reply(soap, ResponseMessageID, ResponseAction);
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Unsubscribe(struct soap* soap, struct _wsnt__Unsubscribe *wsnt__Unsubscribe, struct _wsnt__UnsubscribeResponse *wsnt__UnsubscribeResponse)
{
	DBG_TRACE("### __tev__Unsubscribe ###");
	
	if (soap_wsa_check(soap))
	{
    	return soap->error;
    }
    
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	Bridge_DeleteMotionEvent(soap->path + strlen("/onvif/pullpoint/"));
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__PauseSubscription(struct soap* soap, struct _wsnt__PauseSubscription *wsnt__PauseSubscription, struct _wsnt__PauseSubscriptionResponse *wsnt__PauseSubscriptionResponse)
{
	DBG_TRACE("### __tev__PauseSubscription ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Seek(struct soap* soap, struct _tev__Seek *tev__Seek, struct _tev__SeekResponse *tev__SeekResponse)
{
	DBG_TRACE("### __tev__Seek ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__ResumeSubscription(struct soap* soap, struct _wsnt__ResumeSubscription *wsnt__ResumeSubscription, struct _wsnt__ResumeSubscriptionResponse *wsnt__ResumeSubscriptionResponse)
{
	DBG_TRACE("### __tev__ResumeSubscription ###");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Renew(struct soap* soap, struct _wsnt__Renew *wsnt__Renew, struct _wsnt__RenewResponse *wsnt__RenewResponse)
{
	DBG_TRACE("### __tev__Renew ###");
	
	if (soap_wsa_check(soap))
	{
    	return soap->error;
    }
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("TerminationTime = %s", wsnt__Renew->TerminationTime);
	
	Bridge_RenewMotionEvent(soap->path + strlen("/onvif/pullpoint/"));
	
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Notify_(struct soap* soap, struct _wsnt__Notify *wsnt__Notify)
{
	DBG_TRACE("### __tev__Notify_### ");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__PullMessages(struct soap* soap, struct _tev__PullMessages *tev__PullMessages, struct _tev__PullMessagesResponse *tev__PullMessagesResponse)
{
	DBG_TRACE("### __tev__PullMessages ###");

	if (soap_wsa_check(soap))
	{
    	return soap->error;
    }
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("Timeout      = %d", tev__PullMessages->Timeout);
	DBG_PRINT("MessageLimit = %d", tev__PullMessages->MessageLimit);
	
	char video_source_token[80];
	int motion_detect;
	if (Bridge_WaitMotionEvent(soap->path + strlen("/onvif/pullpoint/"), tev__PullMessages->Timeout, video_source_token, &motion_detect) != 0)
	{
		return soap_receiver_fault(soap, "could not pull message", soap_strdup(soap, "not correct pullpoint"));
	}
	
	tev__PullMessagesResponse->CurrentTime = time(NULL);
	tev__PullMessagesResponse->TerminationTime = time(NULL);
	
	tev__PullMessagesResponse->__sizeNotificationMessage = 1;
	tev__PullMessagesResponse->wsnt__NotificationMessage = soap_new_wsnt__NotificationMessageHolderType(soap, 1);
	
	tev__PullMessagesResponse->wsnt__NotificationMessage->Topic = soap_new_wsnt__TopicExpressionType(soap, 1);
	tev__PullMessagesResponse->wsnt__NotificationMessage->Topic->__any = soap_strdup(soap, "tns1:VideoSource/MotionAlarm");
	tev__PullMessagesResponse->wsnt__NotificationMessage->Topic->Dialect = soap_strdup(soap, "http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet");

	// ----------------------------------------------------------
	
	struct _tt__Message* message = soap_new__tt__Message(soap, 1);

	message->Source = soap_new_tt__ItemList(soap, 1);
	message->Source->__sizeSimpleItem = 1;
	message->Source->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
	message->Source->SimpleItem->Name = soap_strdup(soap, "VideoSourceToken");
	message->Source->SimpleItem->Value = soap_strdup(soap, video_source_token);
	
	message->Data = soap_new_tt__ItemList(soap, 1);
	message->Data->__sizeSimpleItem = 1;
	message->Data->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
	message->Data->SimpleItem->Name = soap_strdup(soap, "State");
	
	if (motion_detect == 1)
	{
		DBG_PRINT("+++ State = True +++");
		message->Data->SimpleItem->Value = soap_strdup(soap, "True");
	}
	else
	{
		DBG_PRINT("+++ State = False +++");
		message->Data->SimpleItem->Value = soap_strdup(soap, "False");
	}
	
	message->UtcTime = time(NULL);

	// ----------------------------------------------------------
	
	struct soap soap_x; 
	soap_init(&soap_x);
	
	soap_begin(&soap_x);
	soap_serialize__tt__Message(&soap_x, message);

	const char* buffer;
	soap_x.os = &buffer;
	soap_set_mode(&soap_x, SOAP_IO_STORE | SOAP_XML_CANONICAL);
	soap_alloc_block(&soap_x);
	
	soap_put__tt__Message(&soap_x, message, NULL, NULL); 
	soap_end_send(&soap_x);

	tev__PullMessagesResponse->wsnt__NotificationMessage->Message.__any = soap_strdup(soap, buffer);
	
	soap_x.os = NULL; // stop sending to string 
	
	soap_end(&soap_x); // remove deserialized data 
	soap_done(&soap_x); // finalize last use of this context

	// ----------------------------------------------------------
	
	const char *ResponseMessageID = soap_wsa_rand_uuid(soap);
	const char *ResponseAction = "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesResponse";

	return soap_wsa_reply(soap, ResponseMessageID, ResponseAction);
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Unsubscribe_(struct soap* soap, struct _wsnt__Unsubscribe *wsnt__Unsubscribe, struct _wsnt__UnsubscribeResponse *wsnt__UnsubscribeResponse)
{
	DBG_TRACE("__tev__Unsubscribe_");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__DestroyPullPoint(struct soap* soap, struct _wsnt__DestroyPullPoint *wsnt__DestroyPullPoint, struct _wsnt__DestroyPullPointResponse *wsnt__DestroyPullPointResponse)
{
	DBG_TRACE("__tev__DestroyPullPoint");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__GetCurrentMessage(struct soap* soap, struct _wsnt__GetCurrentMessage *wsnt__GetCurrentMessage, struct _wsnt__GetCurrentMessageResponse *wsnt__GetCurrentMessageResponse)
{
	DBG_TRACE("__tev__GetCurrentMessage");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Notify(struct soap* soap, struct _wsnt__Notify *wsnt__Notify)
{
	DBG_TRACE("__tev__Notify");
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__GetServiceCapabilities(struct soap* soap, struct _tev__GetServiceCapabilities *tev__GetServiceCapabilities, struct _tev__GetServiceCapabilitiesResponse *tev__GetServiceCapabilitiesResponse)
{
	DBG_TRACE("__tev__GetServiceCapabilities");
	return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// #### RECORDING ####
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/** Web service operation '__trc__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetServiceCapabilities(struct soap* soap, struct _trc__GetServiceCapabilities *trc__GetServiceCapabilities, struct _trc__GetServiceCapabilitiesResponse *trc__GetServiceCapabilitiesResponse)
{
	DBG_TRACE("### __trc__GetServiceCapabilities ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trc__GetServiceCapabilitiesResponse->Capabilities = soap_new_trc__Capabilities(soap, 1);
	
	return 0;
}

/** Web service operation '__trc__CreateRecording' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateRecording(struct soap* soap, struct _trc__CreateRecording *trc__CreateRecording, struct _trc__CreateRecordingResponse *trc__CreateRecordingResponse)
{
	DBG_TRACE("### __trc__CreateRecording ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__DeleteRecording' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteRecording(struct soap* soap, struct _trc__DeleteRecording *trc__DeleteRecording, struct _trc__DeleteRecordingResponse *trc__DeleteRecordingResponse)
{
	DBG_TRACE("### __trc__DeleteRecording ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__GetRecordings' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordings(struct soap* soap, struct _trc__GetRecordings *trc__GetRecordings, struct _trc__GetRecordingsResponse *trc__GetRecordingsResponse)
{
	DBG_TRACE("### __trc__GetRecordings ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	trc__GetRecordingsResponse->__sizeRecordingItem = 0;
	trc__GetRecordingsResponse->RecordingItem = soap_new_tt__GetRecordingsResponseItem(soap, 1);
	trc__GetRecordingsResponse->RecordingItem->RecordingToken = soap_strdup(soap, "RecordingToken1");
	
	trc__GetRecordingsResponse->RecordingItem->Configuration = soap_new_tt__RecordingConfiguration(soap, 1);
	
	trc__GetRecordingsResponse->RecordingItem->Configuration->Source = soap_new_tt__RecordingSourceInformation(soap, 1);
	trc__GetRecordingsResponse->RecordingItem->Configuration->Source->SourceId = soap_strdup(soap, "SourceId1");
	trc__GetRecordingsResponse->RecordingItem->Configuration->Source->Name     = soap_strdup(soap, "RecordingSource");
	trc__GetRecordingsResponse->RecordingItem->Configuration->Source->Location = soap_strdup(soap, "");
	trc__GetRecordingsResponse->RecordingItem->Configuration->Source->Description = soap_strdup(soap, "");
	trc__GetRecordingsResponse->RecordingItem->Configuration->Source->Address = soap_strdup(soap, "");
	trc__GetRecordingsResponse->RecordingItem->Configuration->Content = soap_strdup(soap, "");
	trc__GetRecordingsResponse->RecordingItem->Configuration->MaximumRetentionTime = 24 * 60 * 60 * 1000;
	
	trc__GetRecordingsResponse->RecordingItem->Tracks = soap_new_tt__GetTracksResponseList(soap, 1);
	trc__GetRecordingsResponse->RecordingItem->Tracks->__sizeTrack = 1;
	
	trc__GetRecordingsResponse->RecordingItem->Tracks->Track = soap_new_tt__GetTracksResponseItem(soap, 1);
	trc__GetRecordingsResponse->RecordingItem->Tracks->Track->TrackToken = soap_strdup(soap, "VIDE001");
	trc__GetRecordingsResponse->RecordingItem->Tracks->Track->Configuration = soap_new_tt__TrackConfiguration(soap, 1);
	trc__GetRecordingsResponse->RecordingItem->Tracks->Track->Configuration->TrackType = tt__TrackType__Video;
	trc__GetRecordingsResponse->RecordingItem->Tracks->Track->Configuration->Description = soap_strdup(soap, "");
		
	return 0;
}

/** Web service operation '__trc__SetRecordingConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingConfiguration(struct soap* soap, struct _trc__SetRecordingConfiguration *trc__SetRecordingConfiguration, struct _trc__SetRecordingConfigurationResponse *trc__SetRecordingConfigurationResponse)
{
	DBG_TRACE("### __trc__SetRecordingConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__GetRecordingConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingConfiguration(struct soap* soap, struct _trc__GetRecordingConfiguration *trc__GetRecordingConfiguration, struct _trc__GetRecordingConfigurationResponse *trc__GetRecordingConfigurationResponse)
{
	DBG_TRACE("### __trc__GetRecordingConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("RecordingToken = %s", trc__GetRecordingConfiguration->RecordingToken);
	
	if (strcmp(trc__GetRecordingConfiguration->RecordingToken, "RecordingToken1") == 0)
	{
		trc__GetRecordingConfigurationResponse->RecordingConfiguration = soap_new_tt__RecordingConfiguration(soap, 1);
		trc__GetRecordingConfigurationResponse->RecordingConfiguration ->Source = soap_new_tt__RecordingSourceInformation(soap, 1);
		trc__GetRecordingConfigurationResponse->RecordingConfiguration ->Source->SourceId = soap_strdup(soap, "SourceId1");
		trc__GetRecordingConfigurationResponse->RecordingConfiguration ->Source->Name     = soap_strdup(soap, "RecordingSource");
		trc__GetRecordingConfigurationResponse->RecordingConfiguration ->Source->Location = soap_strdup(soap, "");
		trc__GetRecordingConfigurationResponse->RecordingConfiguration ->Source->Description = soap_strdup(soap, "");
		trc__GetRecordingConfigurationResponse->RecordingConfiguration ->Source->Address = soap_strdup(soap, "");
		trc__GetRecordingConfigurationResponse->RecordingConfiguration ->Content = soap_strdup(soap, "");
		trc__GetRecordingConfigurationResponse->RecordingConfiguration ->MaximumRetentionTime = 24 * 60 * 60 * 1000;
	}
	
	return 0;
}

/** Web service operation '__trc__GetRecordingOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingOptions(struct soap* soap, struct _trc__GetRecordingOptions *trc__GetRecordingOptions, struct _trc__GetRecordingOptionsResponse *trc__GetRecordingOptionsResponse)
{
	DBG_TRACE("### __trc__GetRecordingOptions ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__CreateTrack' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateTrack(struct soap* soap, struct _trc__CreateTrack *trc__CreateTrack, struct _trc__CreateTrackResponse *trc__CreateTrackResponse)
{
	DBG_TRACE("### __trc__CreateTrack ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__DeleteTrack' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteTrack(struct soap* soap, struct _trc__DeleteTrack *trc__DeleteTrack, struct _trc__DeleteTrackResponse *trc__DeleteTrackResponse)
{
	DBG_TRACE("### __trc__DeleteTrack ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__GetTrackConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetTrackConfiguration(struct soap* soap, struct _trc__GetTrackConfiguration *trc__GetTrackConfiguration, struct _trc__GetTrackConfigurationResponse *trc__GetTrackConfigurationResponse)
{
	DBG_TRACE("### __trc__GetTrackConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__SetTrackConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__SetTrackConfiguration(struct soap* soap, struct _trc__SetTrackConfiguration *trc__SetTrackConfiguration, struct _trc__SetTrackConfigurationResponse *trc__SetTrackConfigurationResponse)
{
	DBG_TRACE("### __trc__SetTrackConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__CreateRecordingJob' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateRecordingJob(struct soap* soap, struct _trc__CreateRecordingJob *trc__CreateRecordingJob, struct _trc__CreateRecordingJobResponse *trc__CreateRecordingJobResponse)
{
	DBG_TRACE("### __trc__CreateRecordingJob ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__DeleteRecordingJob' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteRecordingJob(struct soap* soap, struct _trc__DeleteRecordingJob *trc__DeleteRecordingJob, struct _trc__DeleteRecordingJobResponse *trc__DeleteRecordingJobResponse)
{
	DBG_TRACE("### __trc__DeleteRecordingJob ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__GetRecordingJobs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobs(struct soap* soap, struct _trc__GetRecordingJobs *trc__GetRecordingJobs, struct _trc__GetRecordingJobsResponse *trc__GetRecordingJobsResponse)
{
	DBG_TRACE("### __trc__GetRecordingJobs ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	trc__GetRecordingJobsResponse->__sizeJobItem = 1;
	
	trc__GetRecordingJobsResponse->JobItem = soap_new_tt__GetRecordingJobsResponseItem(soap, 1);
	trc__GetRecordingJobsResponse->JobItem->JobToken = soap_strdup(soap, "JobToken1");
	
	trc__GetRecordingJobsResponse->JobItem->JobConfiguration = soap_new_tt__RecordingJobConfiguration(soap, 1);
	trc__GetRecordingJobsResponse->JobItem->JobConfiguration->RecordingToken = soap_strdup(soap, "RecordingToken1");
	
	if (Bridge_GetRecordingMode() == 0)
	{
		trc__GetRecordingJobsResponse->JobItem->JobConfiguration->Mode = soap_strdup(soap, "Idle");
	}
	else
	{
		trc__GetRecordingJobsResponse->JobItem->JobConfiguration->Mode = soap_strdup(soap, "Active");
	}
	
	trc__GetRecordingJobsResponse->JobItem->JobConfiguration->Priority       = 1;

	trc__GetRecordingJobsResponse->JobItem->JobConfiguration->__sizeSource = 1;
	trc__GetRecordingJobsResponse->JobItem->JobConfiguration->Source = soap_new_tt__RecordingJobSource(soap, 1);
	
	trc__GetRecordingJobsResponse->JobItem->JobConfiguration->Source->AutoCreateReceiver = (enum xsd__boolean*)soap_malloc(soap, sizeof(enum xsd__boolean));
	*(trc__GetRecordingJobsResponse->JobItem->JobConfiguration->Source->AutoCreateReceiver) = xsd__boolean__true_;
	
	return 0;
}

/** Web service operation '__trc__SetRecordingJobConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingJobConfiguration(struct soap* soap, struct _trc__SetRecordingJobConfiguration *trc__SetRecordingJobConfiguration, struct _trc__SetRecordingJobConfigurationResponse *trc__SetRecordingJobConfigurationResponse)
{
	DBG_TRACE("### __trc__SetRecordingJobConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("JobToken = %s", trc__SetRecordingJobConfiguration->JobToken);
	DBG_PRINT("Mode     = %s", trc__SetRecordingJobConfiguration->JobConfiguration->Mode);
	
	if (strcmp(trc__SetRecordingJobConfiguration->JobToken, "JobToken1") == 0)
	{
		if (strcmp(trc__SetRecordingJobConfiguration->JobConfiguration->Mode, "Active") == 0)
		{
			Bridge_SetRecordingMode(1);
		}
		else
		{
			Bridge_SetRecordingMode(0);
		}
	}
	
	return 0;
}

/** Web service operation '__trc__GetRecordingJobConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobConfiguration(struct soap* soap, struct _trc__GetRecordingJobConfiguration *trc__GetRecordingJobConfiguration, struct _trc__GetRecordingJobConfigurationResponse *trc__GetRecordingJobConfigurationResponse)
{
	DBG_TRACE("### __trc__GetRecordingJobConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("JobToken = %s", trc__GetRecordingJobConfiguration->JobToken);
	
	if (strcmp(trc__GetRecordingJobConfiguration->JobToken, "JobToken1") == 0)
	{
		trc__GetRecordingJobConfigurationResponse->JobConfiguration = soap_new_tt__RecordingJobConfiguration(soap, 1);
		trc__GetRecordingJobConfigurationResponse->JobConfiguration->RecordingToken = soap_strdup(soap, "RecordingToken1");
		
		if (Bridge_GetRecordingMode() == 0)
		{
			trc__GetRecordingJobConfigurationResponse->JobConfiguration->Mode = soap_strdup(soap, "Idle");
		}
		else
		{
			trc__GetRecordingJobConfigurationResponse->JobConfiguration->Mode = soap_strdup(soap, "Active");
		}
		
		trc__GetRecordingJobConfigurationResponse->JobConfiguration->Priority       = 1;

		trc__GetRecordingJobConfigurationResponse->JobConfiguration->__sizeSource = 1;
		trc__GetRecordingJobConfigurationResponse->JobConfiguration->Source = soap_new_tt__RecordingJobSource(soap, 1);
		
		trc__GetRecordingJobConfigurationResponse->JobConfiguration->Source->AutoCreateReceiver = (enum xsd__boolean*)soap_malloc(soap, sizeof(enum xsd__boolean));
		*(trc__GetRecordingJobConfigurationResponse->JobConfiguration->Source->AutoCreateReceiver) = xsd__boolean__true_;
	}
	
	return 0;
}

/** Web service operation '__trc__SetRecordingJobMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingJobMode(struct soap* soap, struct _trc__SetRecordingJobMode *trc__SetRecordingJobMode, struct _trc__SetRecordingJobModeResponse *trc__SetRecordingJobModeResponse)
{
	DBG_TRACE("### __trc__SetRecordingJobMode ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("JobToken = %s", trc__SetRecordingJobMode->JobToken);
	DBG_PRINT("Mode     = %s", trc__SetRecordingJobMode->Mode);
	
	if (strcmp(trc__SetRecordingJobMode->JobToken, "JobToken1") == 0)
	{
		if (strcmp(trc__SetRecordingJobMode->Mode, "Active") == 0)
		{
			Bridge_StartRecording();
		}
		else if (strcmp(trc__SetRecordingJobMode->Mode, "Idle") == 0)
		{
			Bridge_StopRecording();
		}
	}
	
	return 0;
}

/** Web service operation '__trc__GetRecordingJobState' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobState(struct soap* soap, struct _trc__GetRecordingJobState *trc__GetRecordingJobState, struct _trc__GetRecordingJobStateResponse *trc__GetRecordingJobStateResponse)
{
	DBG_TRACE("### __trc__GetRecordingJobState ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("JobToken = %s", trc__GetRecordingJobState->JobToken);
	
	if (strcmp(trc__GetRecordingJobState->JobToken, "JobToken1") == 0)
	{
		trc__GetRecordingJobStateResponse->State = soap_new_tt__RecordingJobStateInformation(soap, 1);
		trc__GetRecordingJobStateResponse->State->RecordingToken = soap_strdup(soap, "RecordingToken1");

		if (Bridge_IsRecording() == 0)
		{
			trc__GetRecordingJobStateResponse->State->State = soap_strdup(soap, "Idle");
		}
		else
		{
			trc__GetRecordingJobStateResponse->State->State = soap_strdup(soap, "Active");
		}
	}

	return 0;
}

/** Web service operation '__trc__ExportRecordedData' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__ExportRecordedData(struct soap* soap, struct _trc__ExportRecordedData *trc__ExportRecordedData, struct _trc__ExportRecordedDataResponse *trc__ExportRecordedDataResponse)
{
	DBG_TRACE("### __trc__ExportRecordedData ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__StopExportRecordedData' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__StopExportRecordedData(struct soap* soap, struct _trc__StopExportRecordedData *trc__StopExportRecordedData, struct _trc__StopExportRecordedDataResponse *trc__StopExportRecordedDataResponse)
{
	DBG_TRACE("### __trc__StopExportRecordedData ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__trc__GetExportRecordedDataState' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetExportRecordedDataState(struct soap* soap, struct _trc__GetExportRecordedDataState *trc__GetExportRecordedDataState, struct _trc__GetExportRecordedDataStateResponse *trc__GetExportRecordedDataStateResponse)
{
	DBG_TRACE("### __trc__GetExportRecordedDataState ###");
	return SOAP_NO_METHOD;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// #### PTZ ####
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void StorePTZConfiguration(struct soap* soap, struct tt__PTZConfiguration* dst, struct ONVIF_PTZ_CONFIG_INFO* src)
{
	dst->Name      = soap_strdup(soap, src->name);
	dst->UseCount  = src->use_count;
	dst->token     = soap_strdup(soap, src->token);
	dst->NodeToken = soap_strdup(soap, src->node_token);
	dst->DefaultAbsolutePantTiltPositionSpace   = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
	dst->DefaultRelativePanTiltTranslationSpace = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace");
	dst->DefaultContinuousPanTiltVelocitySpace  = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace");
}

/** Web service operation '__tptz__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetServiceCapabilities(struct soap* soap, struct _tptz__GetServiceCapabilities *tptz__GetServiceCapabilities, struct _tptz__GetServiceCapabilitiesResponse *tptz__GetServiceCapabilitiesResponse)
{
	DBG_TRACE("### __tptz__GetServiceCapabilities ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	tptz__GetServiceCapabilitiesResponse->Capabilities = soap_new_tptz__Capabilities(soap, 1);

	tptz__GetServiceCapabilitiesResponse->Capabilities->MoveStatus = (enum xsd__boolean*)soap_malloc(soap, sizeof(enum xsd__boolean));
	*(tptz__GetServiceCapabilitiesResponse->Capabilities->MoveStatus) = xsd__boolean__true_;

	tptz__GetServiceCapabilitiesResponse->Capabilities->StatusPosition = (enum xsd__boolean*)soap_malloc(soap, sizeof(enum xsd__boolean));
	*(tptz__GetServiceCapabilitiesResponse->Capabilities->StatusPosition) = xsd__boolean__true_;
	
	return 0;
}

/** Web service operation '__tptz__GetConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetConfigurations(struct soap* soap, struct _tptz__GetConfigurations *tptz__GetConfigurations, struct _tptz__GetConfigurationsResponse *tptz__GetConfigurationsResponse)
{
	DBG_TRACE("### __tptz__GetConfigurations ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	tptz__GetConfigurationsResponse->__sizePTZConfiguration = Bridge_GetPtzConfigCount();
	tptz__GetConfigurationsResponse->PTZConfiguration = soap_new_tt__PTZConfiguration(soap, tptz__GetConfigurationsResponse->__sizePTZConfiguration);
	
	int i = 0;
	for (i = 0; i < tptz__GetConfigurationsResponse->__sizePTZConfiguration; i++)
	{
		struct ONVIF_PTZ_CONFIG_INFO ptz_config_info;
		Bridge_GetPtzConfig(i, &ptz_config_info);
		
		StorePTZConfiguration(soap, &tptz__GetConfigurationsResponse->PTZConfiguration[i], &ptz_config_info);
	}
	
	return 0;
}

/** Web service operation '__tptz__GetPresets' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetPresets(struct soap* soap, struct _tptz__GetPresets *tptz__GetPresets, struct _tptz__GetPresetsResponse *tptz__GetPresetsResponse)
{
	DBG_TRACE("### __tptz__GetPresets ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__SetPreset' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__SetPreset(struct soap* soap, struct _tptz__SetPreset *tptz__SetPreset, struct _tptz__SetPresetResponse *tptz__SetPresetResponse)
{
	DBG_TRACE("### __tptz__SetPreset ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__RemovePreset' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__RemovePreset(struct soap* soap, struct _tptz__RemovePreset *tptz__RemovePreset, struct _tptz__RemovePresetResponse *tptz__RemovePresetResponse)
{
	DBG_TRACE("### __tptz__RemovePreset ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__GotoPreset' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GotoPreset(struct soap* soap, struct _tptz__GotoPreset *tptz__GotoPreset, struct _tptz__GotoPresetResponse *tptz__GotoPresetResponse)
{
	DBG_TRACE("### __tptz__GotoPreset ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__GetStatus' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetStatus(struct soap* soap, struct _tptz__GetStatus *tptz__GetStatus, struct _tptz__GetStatusResponse *tptz__GetStatusResponse)
{
	DBG_TRACE("### __tptz__GetStatus ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("ProfileToken = %s", tptz__GetStatus->ProfileToken);
	
	float x = 0;
	float y = 0;
	int is_moving = 0;
	Bridge_GetPtzStatus(tptz__GetStatus->ProfileToken, &x, &y, &is_moving);
	
	tptz__GetStatusResponse->PTZStatus = soap_new_tt__PTZStatus(soap, 1);

	tptz__GetStatusResponse->PTZStatus->Position = soap_new_tt__PTZVector(soap, 1);
	tptz__GetStatusResponse->PTZStatus->Position->PanTilt = soap_new_tt__Vector2D(soap, 1);
	tptz__GetStatusResponse->PTZStatus->Position->PanTilt->x = x;
	tptz__GetStatusResponse->PTZStatus->Position->PanTilt->y = y;
	tptz__GetStatusResponse->PTZStatus->Position->PanTilt->space = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
	
	tptz__GetStatusResponse->PTZStatus->MoveStatus = soap_new_tt__PTZMoveStatus(soap, 1);
	tptz__GetStatusResponse->PTZStatus->MoveStatus->PanTilt = soap_new_tt__MoveStatus(soap, 1);
	if (is_moving == 0)
	{
		tptz__GetStatusResponse->PTZStatus->MoveStatus->PanTilt[0] = tt__MoveStatus__IDLE;
	}
	else
	{
		tptz__GetStatusResponse->PTZStatus->MoveStatus->PanTilt[0] = tt__MoveStatus__MOVING;
	}
	
	tptz__GetStatusResponse->PTZStatus->UtcTime = time(NULL);
	
	return 0;
}

/** Web service operation '__tptz__GetConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetConfiguration(struct soap* soap, struct _tptz__GetConfiguration *tptz__GetConfiguration, struct _tptz__GetConfigurationResponse *tptz__GetConfigurationResponse)
{
	DBG_TRACE("### __tptz__GetConfiguration ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("PTZConfigurationToken = %s", tptz__GetConfiguration->PTZConfigurationToken);
	
	struct ONVIF_PTZ_CONFIG_INFO ptz_config_info;
	if (Bridge_FindPtzConfig(tptz__GetConfiguration->PTZConfigurationToken, &ptz_config_info) == 0)
	{
		tptz__GetConfigurationResponse->PTZConfiguration = soap_new_tt__PTZConfiguration(soap, 1);
		
		StorePTZConfiguration(soap, tptz__GetConfigurationResponse->PTZConfiguration, &ptz_config_info);
	}
	
	return 0;
}

/** Web service operation '__tptz__GetNodes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetNodes(struct soap* soap, struct _tptz__GetNodes *tptz__GetNodes, struct _tptz__GetNodesResponse *tptz__GetNodesResponse)
{
	DBG_TRACE("### __tptz__GetNodes ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	if (Bridge_IsSupportedPTZ() == 1)
	{
		tptz__GetNodesResponse->__sizePTZNode = 1;
		
		tptz__GetNodesResponse->PTZNode = soap_new_tt__PTZNode(soap, 1);
		tptz__GetNodesResponse->PTZNode->token = soap_strdup(soap, "PtzNodeToken1");

		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces = soap_new_tt__PTZSpaces(soap, 1);
		
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeAbsolutePanTiltPositionSpace = 1;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace = soap_new_tt__Space2DDescription(soap, 1);

		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].URI = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].XRange = soap_new_tt__FloatRange(soap, 1);
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].XRange->Min = -1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].XRange->Max = 1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].YRange = soap_new_tt__FloatRange(soap, 1);
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].YRange->Min = -1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].YRange->Max = 1.0;

		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeRelativePanTiltTranslationSpace = 1;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace = soap_new_tt__Space2DDescription(soap, 1);

		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].URI = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace");
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].XRange = soap_new_tt__FloatRange(soap, 1);
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].XRange->Min = -1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].XRange->Max = 1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].YRange = soap_new_tt__FloatRange(soap, 1);
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].YRange->Min = -1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].YRange->Max = 1.0;

		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizeContinuousPanTiltVelocitySpace = 1;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace = soap_new_tt__Space2DDescription(soap, 1);

		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].URI = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace");
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].XRange = soap_new_tt__FloatRange(soap, 1);
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].XRange->Min = -1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].XRange->Max = 1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].YRange = soap_new_tt__FloatRange(soap, 1);
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].YRange->Min = -1.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].YRange->Max = 1.0;

		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->__sizePanTiltSpeedSpace = 1;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace = soap_new_tt__Space1DDescription(soap, 1);

		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace[0].URI = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace");
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace[0].XRange = soap_new_tt__FloatRange(soap, 1);
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace[0].XRange->Min = 0.0;
		tptz__GetNodesResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace[0].XRange->Max = 1.0;
		
		tptz__GetNodesResponse->PTZNode->MaximumNumberOfPresets = 0;
		tptz__GetNodesResponse->PTZNode->HomeSupported = xsd__boolean__true_;
	}
	
	return 0;
}

/** Web service operation '__tptz__GetNode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetNode(struct soap* soap, struct _tptz__GetNode *tptz__GetNode, struct _tptz__GetNodeResponse *tptz__GetNodeResponse)
{
	DBG_TRACE("### __tptz__GetNode ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("NodeToken = %s", tptz__GetNode->NodeToken);

	if (Bridge_IsSupportedPTZ() == 1)
	{
		if (strcmp(tptz__GetNode->NodeToken, "PtzNodeToken1") == 0)
		{
			tptz__GetNodeResponse->PTZNode = soap_new_tt__PTZNode(soap, 1);
			tptz__GetNodeResponse->PTZNode->token = soap_strdup(soap, "PtzNodeToken1");

			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces = soap_new_tt__PTZSpaces(soap, 1);
			
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeAbsolutePanTiltPositionSpace = 1;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace = soap_new_tt__Space2DDescription(soap, 1);

			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].URI = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace");
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].XRange = soap_new_tt__FloatRange(soap, 1);
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].XRange->Min = -1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].XRange->Max = 1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].YRange = soap_new_tt__FloatRange(soap, 1);
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].YRange->Min = -1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->AbsolutePanTiltPositionSpace[0].YRange->Max = 1.0;

			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeRelativePanTiltTranslationSpace = 1;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace = soap_new_tt__Space2DDescription(soap, 1);

			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].URI = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace");
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].XRange = soap_new_tt__FloatRange(soap, 1);
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].XRange->Min = -1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].XRange->Max = 1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].YRange = soap_new_tt__FloatRange(soap, 1);
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].YRange->Min = -1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->RelativePanTiltTranslationSpace[0].YRange->Max = 1.0;

			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizeContinuousPanTiltVelocitySpace = 1;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace = soap_new_tt__Space2DDescription(soap, 1);

			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].URI = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace");
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].XRange = soap_new_tt__FloatRange(soap, 1);
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].XRange->Min = -1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].XRange->Max = 1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].YRange = soap_new_tt__FloatRange(soap, 1);
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].YRange->Min = -1.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace[0].YRange->Max = 1.0;

			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->__sizePanTiltSpeedSpace = 1;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace = soap_new_tt__Space1DDescription(soap, 1);

			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace[0].URI = soap_strdup(soap, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace");
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace[0].XRange = soap_new_tt__FloatRange(soap, 1);
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace[0].XRange->Min = 0.0;
			tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->PanTiltSpeedSpace[0].XRange->Max = 1.0;
			
			tptz__GetNodeResponse->PTZNode->MaximumNumberOfPresets = 0;
			tptz__GetNodeResponse->PTZNode->HomeSupported = xsd__boolean__true_;
		}
	}
	
	return 0;
}

/** Web service operation '__tptz__SetConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__SetConfiguration(struct soap* soap, struct _tptz__SetConfiguration *tptz__SetConfiguration, struct _tptz__SetConfigurationResponse *tptz__SetConfigurationResponse)
{
	DBG_TRACE("### __tptz__SetConfiguration ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__GetConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetConfigurationOptions(struct soap* soap, struct _tptz__GetConfigurationOptions *tptz__GetConfigurationOptions, struct _tptz__GetConfigurationOptionsResponse *tptz__GetConfigurationOptionsResponse)
{
	DBG_TRACE("### __tptz__GetConfigurationOptions ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__GotoHomePosition' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GotoHomePosition(struct soap* soap, struct _tptz__GotoHomePosition *tptz__GotoHomePosition, struct _tptz__GotoHomePositionResponse *tptz__GotoHomePositionResponse)
{
	DBG_TRACE("### __tptz__GotoHomePosition ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s", tptz__GotoHomePosition->ProfileToken);
	
	float sx = 0;
	float sy = 0;
	
	if (tptz__GotoHomePosition->Speed != NULL)
	{
		if (tptz__GotoHomePosition->Speed->PanTilt != NULL)
		{
			DBG_PRINT("speed: x = %f", tptz__GotoHomePosition->Speed->PanTilt->x);
			DBG_PRINT("speed: y = %f", tptz__GotoHomePosition->Speed->PanTilt->y);
			
			sx = tptz__GotoHomePosition->Speed->PanTilt->x;
			sy = tptz__GotoHomePosition->Speed->PanTilt->y;
		}
	}

	struct ONVIF_PROFILE_INFO profile_info;
	if (Bridge_FindProfile(tptz__GotoHomePosition->ProfileToken, &profile_info) == 0)
	{
		DBG_PRINT("x = %f", profile_info.ptz_home_pos_x);
		DBG_PRINT("y = %f", profile_info.ptz_home_pos_y);
		
		Bridge_AbsoluteMove(
			tptz__GotoHomePosition->ProfileToken,
			profile_info.ptz_home_pos_x,
			profile_info.ptz_home_pos_y,
			sx,
			sy
			);
	}
	
	return 0;
}

/** Web service operation '__tptz__SetHomePosition' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__SetHomePosition(struct soap* soap, struct _tptz__SetHomePosition *tptz__SetHomePosition, struct _tptz__SetHomePositionResponse *tptz__SetHomePositionResponse)
{
	DBG_TRACE("### __tptz__SetHomePosition ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__Administrator);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s", tptz__SetHomePosition->ProfileToken);
	
	float x = 0;
	float y = 0;
	int is_moving = 0;
	if (Bridge_GetPtzStatus(tptz__SetHomePosition->ProfileToken, &x, &y, &is_moving) != 0)
	{
		return SOAP_ERR;
	}
	DBG_PRINT("x = %f", x);
	DBG_PRINT("y = %f", y);
	
	Bridge_UpdateHomePosition(tptz__SetHomePosition->ProfileToken, x, y);
	
	return 0;
}

/** Web service operation '__tptz__ContinuousMove' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__ContinuousMove(struct soap* soap, struct _tptz__ContinuousMove *tptz__ContinuousMove, struct _tptz__ContinuousMoveResponse *tptz__ContinuousMoveResponse)
{
	DBG_TRACE("### __tptz__ContinuousMove ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("ProfileToken = %s", tptz__ContinuousMove->ProfileToken);
	
	if (tptz__ContinuousMove->Velocity != NULL)
	{
		DBG_PRINT("x = %f", tptz__ContinuousMove->Velocity->PanTilt->x);
		DBG_PRINT("y = %f", tptz__ContinuousMove->Velocity->PanTilt->y);
		
		Bridge_ContinuousMove(
			tptz__ContinuousMove->ProfileToken,
			tptz__ContinuousMove->Velocity->PanTilt->x,
			tptz__ContinuousMove->Velocity->PanTilt->y
			);
	}
	
	return 0;
}

/** Web service operation '__tptz__RelativeMove' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__RelativeMove(struct soap* soap, struct _tptz__RelativeMove *tptz__RelativeMove, struct _tptz__RelativeMoveResponse *tptz__RelativeMoveResponse)
{
	DBG_TRACE("### __tptz__RelativeMove ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("ProfileToken = %s", tptz__RelativeMove->ProfileToken);
	
	float sx = 0;
	float sy = 0;
	
	if (tptz__RelativeMove->Speed != NULL)
	{
		if (tptz__RelativeMove->Speed->PanTilt != NULL)
		{
			DBG_PRINT("speed: x = %f", tptz__RelativeMove->Speed->PanTilt->x);
			DBG_PRINT("speed: y = %f", tptz__RelativeMove->Speed->PanTilt->y);
			
			sx = tptz__RelativeMove->Speed->PanTilt->x;
			sy = tptz__RelativeMove->Speed->PanTilt->y;
		}
	}
	
	if (tptz__RelativeMove->Translation != NULL)
	{
		if (tptz__RelativeMove->Translation->PanTilt != NULL)
		{
			DBG_PRINT("x = %f", tptz__RelativeMove->Translation->PanTilt->x);
			DBG_PRINT("y = %f", tptz__RelativeMove->Translation->PanTilt->y);
			
			Bridge_RelativeMove(
				tptz__RelativeMove->ProfileToken,
				tptz__RelativeMove->Translation->PanTilt->x,
				tptz__RelativeMove->Translation->PanTilt->y,
				sx,
				sy
				);
		}
	}
	
	return 0;
}

/** Web service operation '__tptz__SendAuxiliaryCommand' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__SendAuxiliaryCommand(struct soap* soap, struct _tptz__SendAuxiliaryCommand *tptz__SendAuxiliaryCommand, struct _tptz__SendAuxiliaryCommandResponse *tptz__SendAuxiliaryCommandResponse)
{
	DBG_TRACE("### __tptz__SendAuxiliaryCommand ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__AbsoluteMove' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__AbsoluteMove(struct soap* soap, struct _tptz__AbsoluteMove *tptz__AbsoluteMove, struct _tptz__AbsoluteMoveResponse *tptz__AbsoluteMoveResponse)
{
	DBG_TRACE("### __tptz__AbsoluteMove ###");

	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}

	DBG_PRINT("ProfileToken = %s", tptz__AbsoluteMove->ProfileToken);
	
	float sx = 0;
	float sy = 0;
	
	if (tptz__AbsoluteMove->Speed != NULL)
	{
		if (tptz__AbsoluteMove->Speed->PanTilt != NULL)
		{
			DBG_PRINT("speed: x = %f", tptz__AbsoluteMove->Speed->PanTilt->x);
			DBG_PRINT("speed: y = %f", tptz__AbsoluteMove->Speed->PanTilt->y);
			
			sx = tptz__AbsoluteMove->Speed->PanTilt->x;
			sy = tptz__AbsoluteMove->Speed->PanTilt->y;
		}
	}
	
	if (tptz__AbsoluteMove->Position != NULL)
	{
		if (tptz__AbsoluteMove->Position->PanTilt != NULL)
		{
			DBG_PRINT("x = %f", tptz__AbsoluteMove->Position->PanTilt->x);
			DBG_PRINT("y = %f", tptz__AbsoluteMove->Position->PanTilt->y);
			
			Bridge_AbsoluteMove(
				tptz__AbsoluteMove->ProfileToken,
				tptz__AbsoluteMove->Position->PanTilt->x,
				tptz__AbsoluteMove->Position->PanTilt->y,
				sx,
				sy
				);
		}
	}
	
	return 0;
}

/** Web service operation '__tptz__GeoMove' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GeoMove(struct soap* soap, struct _tptz__GeoMove *tptz__GeoMove, struct _tptz__GeoMoveResponse *tptz__GeoMoveResponse)
{
	DBG_TRACE("### __tptz__GeoMove ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__Stop' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__Stop(struct soap* soap, struct _tptz__Stop *tptz__Stop, struct _tptz__StopResponse *tptz__StopResponse)
{
	DBG_TRACE("### __tptz__Stop ###");
	
	int ret = VerifyPassword(soap, tt__UserLevel__User);
	if (ret != 0)
	{
		return ret;
	}
	
	DBG_PRINT("ProfileToken = %s", tptz__Stop->ProfileToken);
	
	if (tptz__Stop->PanTilt != NULL)
	{
		if (*(tptz__Stop->PanTilt) == xsd__boolean__true_)
		{
			Brdige_StopMove(tptz__Stop->ProfileToken);
		}
	}
	
	return 0;
}

/** Web service operation '__tptz__GetPresetTours' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetPresetTours(struct soap* soap, struct _tptz__GetPresetTours *tptz__GetPresetTours, struct _tptz__GetPresetToursResponse *tptz__GetPresetToursResponse)
{
	DBG_TRACE("### __tptz__GetPresetTours ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__GetPresetTour' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetPresetTour(struct soap* soap, struct _tptz__GetPresetTour *tptz__GetPresetTour, struct _tptz__GetPresetTourResponse *tptz__GetPresetTourResponse)
{
	DBG_TRACE("### __tptz__GetPresetTour ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__GetPresetTourOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetPresetTourOptions(struct soap* soap, struct _tptz__GetPresetTourOptions *tptz__GetPresetTourOptions, struct _tptz__GetPresetTourOptionsResponse *tptz__GetPresetTourOptionsResponse)
{
	DBG_TRACE("### __tptz__GetPresetTour ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__CreatePresetTour' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__CreatePresetTour(struct soap* soap, struct _tptz__CreatePresetTour *tptz__CreatePresetTour, struct _tptz__CreatePresetTourResponse *tptz__CreatePresetTourResponse)
{
	DBG_TRACE("### __tptz__CreatePresetTour ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__ModifyPresetTour' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__ModifyPresetTour(struct soap* soap, struct _tptz__ModifyPresetTour *tptz__ModifyPresetTour, struct _tptz__ModifyPresetTourResponse *tptz__ModifyPresetTourResponse)
{
	DBG_TRACE("### __tptz__ModifyPresetTour ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__OperatePresetTour' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__OperatePresetTour(struct soap* soap, struct _tptz__OperatePresetTour *tptz__OperatePresetTour, struct _tptz__OperatePresetTourResponse *tptz__OperatePresetTourResponse)
{
	DBG_TRACE("### __tptz__OperatePresetTour ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__RemovePresetTour' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__RemovePresetTour(struct soap* soap, struct _tptz__RemovePresetTour *tptz__RemovePresetTour, struct _tptz__RemovePresetTourResponse *tptz__RemovePresetTourResponse)
{
	DBG_TRACE("### __tptz__RemovePresetTour ###");
	return SOAP_NO_METHOD;
}

/** Web service operation '__tptz__GetCompatibleConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetCompatibleConfigurations(struct soap* soap, struct _tptz__GetCompatibleConfigurations *tptz__GetCompatibleConfigurations, struct _tptz__GetCompatibleConfigurationsResponse *tptz__GetCompatibleConfigurationsResponse)
{
	DBG_TRACE("### __tptz__GetCompatibleConfigurations ###");
	return SOAP_NO_METHOD;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int SOAP_ENV__Fault(struct soap *soap, char *faultcode, char *faultstring, char *faultactor, struct SOAP_ENV__Detail *detail, struct SOAP_ENV__Code *SOAP_ENV__Code, struct SOAP_ENV__Reason *SOAP_ENV__Reason, char *SOAP_ENV__Node, char *SOAP_ENV__Role, struct SOAP_ENV__Detail *SOAP_ENV__Detail)
{
	DBG_TRACE("### SOAP_ENV__Fault ###");
	return SOAP_OK;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void * ProcessOnvifRequest(void* soap);

/**
 * ONVIFサービス停止フラグ
 */
static int onvif_terminate = 0;

/**
 * ONVIFサービスを実行する
 */
int RunOnvifService(int port)
{
	onvif_terminate = 0;
	
	// gSoapを初期化する
	struct soap soap; 
	soap_init(&soap); 
	
	// プラグインを登録する
	soap_register_plugin(&soap, soap_wsse);		// WS-Security
	soap_register_plugin(&soap, soap_wsa);		// WS-Address
	
	// ポートリユースを設定。Accept待ちタイムアウトは1秒
	soap.bind_flags=SO_REUSEADDR;
	soap.accept_timeout = 1;
	
	// ポートをバインドする
	int m = soap_bind(&soap, NULL, port, 10); 
   	if (0 <= m) 
   	{ 
      	while (onvif_terminate == 0)
      	{
      		// 接続待ちを行う
			int s = soap_accept(&soap); 
         	if (0 <= s) 
         	{
         		// gSoapコンテキストをコピーする
				struct soap* tsoap;
				tsoap = soap_copy(&soap);
				// ONVIFプロトコルを処理するスレッドを起動する
				pthread_t tid;
			    pthread_create(&tid, NULL, ProcessOnvifRequest, tsoap);
		    }
      	} 
	} 
	else
	{
    	soap_print_fault(&soap, stderr); 
  	}
	
	// gSoapを終了する
	soap_done(&soap);
	
	return 0;
}

/**
 * ONVIFサービスを停止する
 */
void StopOnvifService()
{
	onvif_terminate = 1;
}

/**
 * ONVIFプロトコルを処理する
 * ※スレッドで実行されるので、複数同時に実行される可能性がある。
 */
static void * ProcessOnvifRequest(void* param)
{
	// パラメータをキャストする
	struct soap* soap = (struct soap*)param;
	
	// 処理終了時にスレッドのリソースを自動で解放する
	pthread_detach(pthread_self());

	// ONVIFプロトコルを処理する
	soap_serve(soap);
	
	// ONVIFリソースを解放する
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return NULL;
}

//------------------------------------------------------------------------------------------------------------------------------------------------

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_request(struct soap *soap)
{
	soap_peek_element(soap);

	if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "SOAP-ENV:Fault")) || (soap->action && !strcmp(soap->action, "http://www.w3.org/2005/08/addressing/soap/fault")))
		return soap_serve_SOAP_ENV__Fault(soap);
	
	if (soap->path != NULL)
	{
		if (strcmp(soap->path, "/onvif/device_service") == 0)
		{
			DBG_PRINT("+++ DEVICE +++");

			if (soap->action != NULL)
			{
				DBG_PRINT("ACTION = %s", soap->action);
			}
			else
			{
				DBG_PRINT("TAG = %s", soap->tag);
			}
			
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetServices")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetServices")))
				return soap_serve___tds__GetServices(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetServiceCapabilities")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetServiceCapabilities")))
				return soap_serve___tds__GetServiceCapabilities(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDeviceInformation")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDeviceInformation")))
				return soap_serve___tds__GetDeviceInformation(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetSystemDateAndTime")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetSystemDateAndTime")))
				return soap_serve___tds__SetSystemDateAndTime(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetSystemDateAndTime")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetSystemDateAndTime")))
				return soap_serve___tds__GetSystemDateAndTime(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetSystemFactoryDefault")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetSystemFactoryDefault")))
				return soap_serve___tds__SetSystemFactoryDefault(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:UpgradeSystemFirmware")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/UpgradeSystemFirmware")))
				return soap_serve___tds__UpgradeSystemFirmware(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SystemReboot")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SystemReboot")))
				return soap_serve___tds__SystemReboot(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:RestoreSystem")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/RestoreSystem")))
				return soap_serve___tds__RestoreSystem(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetSystemBackup")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetSystemBackup")))
				return soap_serve___tds__GetSystemBackup(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetSystemLog")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetSystemLog")))
				return soap_serve___tds__GetSystemLog(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetSystemSupportInformation")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetSystemSupportInformation")))
				return soap_serve___tds__GetSystemSupportInformation(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetScopes")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetScopes")))
				return soap_serve___tds__GetScopes(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetScopes")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetScopes")))
				return soap_serve___tds__SetScopes(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:AddScopes")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/AddScopes")))
				return soap_serve___tds__AddScopes(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:RemoveScopes")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/RemoveScopes")))
				return soap_serve___tds__RemoveScopes(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDiscoveryMode")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDiscoveryMode")))
				return soap_serve___tds__GetDiscoveryMode(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetDiscoveryMode")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetDiscoveryMode")))
				return soap_serve___tds__SetDiscoveryMode(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetRemoteDiscoveryMode")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetRemoteDiscoveryMode")))
				return soap_serve___tds__GetRemoteDiscoveryMode(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetRemoteDiscoveryMode")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetRemoteDiscoveryMode")))
				return soap_serve___tds__SetRemoteDiscoveryMode(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDPAddresses")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDPAddresses")))
				return soap_serve___tds__GetDPAddresses(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetEndpointReference")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetEndpointReference")))
				return soap_serve___tds__GetEndpointReference(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetRemoteUser")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetRemoteUser")))
				return soap_serve___tds__GetRemoteUser(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetRemoteUser")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetRemoteUser")))
				return soap_serve___tds__SetRemoteUser(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetUsers")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetUsers")))
				return soap_serve___tds__GetUsers(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:CreateUsers")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/CreateUsers")))
				return soap_serve___tds__CreateUsers(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:DeleteUsers")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/DeleteUsers")))
				return soap_serve___tds__DeleteUsers(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetUser")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetUser")))
				return soap_serve___tds__SetUser(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetWsdlUrl")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetWsdlUrl")))
				return soap_serve___tds__GetWsdlUrl(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetCapabilities")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetCapabilities")))
				return soap_serve___tds__GetCapabilities(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetDPAddresses")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetDPAddresses")))
				return soap_serve___tds__SetDPAddresses(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetHostname")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetHostname")))
				return soap_serve___tds__GetHostname(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetHostname")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetHostname")))
				return soap_serve___tds__SetHostname(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetHostnameFromDHCP")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetHostnameFromDHCP")))
				return soap_serve___tds__SetHostnameFromDHCP(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDNS")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDNS")))
				return soap_serve___tds__GetDNS(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetDNS")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetDNS")))
				return soap_serve___tds__SetDNS(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetNTP")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetNTP")))
				return soap_serve___tds__GetNTP(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetNTP")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetNTP")))
				return soap_serve___tds__SetNTP(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDynamicDNS")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDynamicDNS")))
				return soap_serve___tds__GetDynamicDNS(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetDynamicDNS")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetDynamicDNS")))
				return soap_serve___tds__SetDynamicDNS(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetNetworkInterfaces")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetNetworkInterfaces")))
				return soap_serve___tds__GetNetworkInterfaces(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetNetworkInterfaces")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetNetworkInterfaces")))
				return soap_serve___tds__SetNetworkInterfaces(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetNetworkProtocols")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetNetworkProtocols")))
				return soap_serve___tds__GetNetworkProtocols(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetNetworkProtocols")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetNetworkProtocols")))
				return soap_serve___tds__SetNetworkProtocols(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetNetworkDefaultGateway")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetNetworkDefaultGateway")))
				return soap_serve___tds__GetNetworkDefaultGateway(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetNetworkDefaultGateway")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetNetworkDefaultGateway")))
				return soap_serve___tds__SetNetworkDefaultGateway(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetZeroConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetZeroConfiguration")))
				return soap_serve___tds__GetZeroConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetZeroConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetZeroConfiguration")))
				return soap_serve___tds__SetZeroConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetIPAddressFilter")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetIPAddressFilter")))
				return soap_serve___tds__GetIPAddressFilter(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetIPAddressFilter")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetIPAddressFilter")))
				return soap_serve___tds__SetIPAddressFilter(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:AddIPAddressFilter")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/AddIPAddressFilter")))
				return soap_serve___tds__AddIPAddressFilter(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:RemoveIPAddressFilter")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/RemoveIPAddressFilter")))
				return soap_serve___tds__RemoveIPAddressFilter(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetAccessPolicy")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetAccessPolicy")))
				return soap_serve___tds__GetAccessPolicy(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetAccessPolicy")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetAccessPolicy")))
				return soap_serve___tds__SetAccessPolicy(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:CreateCertificate")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/CreateCertificate")))
				return soap_serve___tds__CreateCertificate(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetCertificates")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetCertificates")))
				return soap_serve___tds__GetCertificates(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetCertificatesStatus")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetCertificatesStatus")))
				return soap_serve___tds__GetCertificatesStatus(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetCertificatesStatus")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetCertificatesStatus")))
				return soap_serve___tds__SetCertificatesStatus(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:DeleteCertificates")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/DeleteCertificates")))
				return soap_serve___tds__DeleteCertificates(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetPkcs10Request")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetPkcs10Request")))
				return soap_serve___tds__GetPkcs10Request(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:LoadCertificates")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/LoadCertificates")))
				return soap_serve___tds__LoadCertificates(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetClientCertificateMode")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetClientCertificateMode")))
				return soap_serve___tds__GetClientCertificateMode(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetClientCertificateMode")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetClientCertificateMode")))
				return soap_serve___tds__SetClientCertificateMode(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetRelayOutputs")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetRelayOutputs")))
				return soap_serve___tds__GetRelayOutputs(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetRelayOutputSettings")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetRelayOutputSettings")))
				return soap_serve___tds__SetRelayOutputSettings(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetRelayOutputState")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetRelayOutputState")))
				return soap_serve___tds__SetRelayOutputState(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SendAuxiliaryCommand")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SendAuxiliaryCommand")))
				return soap_serve___tds__SendAuxiliaryCommand(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetCACertificates")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetCACertificates")))
				return soap_serve___tds__GetCACertificates(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:LoadCertificateWithPrivateKey")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/LoadCertificateWithPrivateKey")))
				return soap_serve___tds__LoadCertificateWithPrivateKey(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetCertificateInformation")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetCertificateInformation")))
				return soap_serve___tds__GetCertificateInformation(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:LoadCACertificates")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/LoadCACertificates")))
				return soap_serve___tds__LoadCACertificates(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:CreateDot1XConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/CreateDot1XConfiguration")))
				return soap_serve___tds__CreateDot1XConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetDot1XConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetDot1XConfiguration")))
				return soap_serve___tds__SetDot1XConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDot1XConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDot1XConfiguration")))
				return soap_serve___tds__GetDot1XConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDot1XConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDot1XConfigurations")))
				return soap_serve___tds__GetDot1XConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:DeleteDot1XConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/DeleteDot1XConfiguration")))
				return soap_serve___tds__DeleteDot1XConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDot11Capabilities")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDot11Capabilities")))
				return soap_serve___tds__GetDot11Capabilities(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetDot11Status")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetDot11Status")))
				return soap_serve___tds__GetDot11Status(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:ScanAvailableDot11Networks")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/ScanAvailableDot11Networks")))
				return soap_serve___tds__ScanAvailableDot11Networks(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetSystemUris")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetSystemUris")))
				return soap_serve___tds__GetSystemUris(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:StartFirmwareUpgrade")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/StartFirmwareUpgrade")))
				return soap_serve___tds__StartFirmwareUpgrade(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:StartSystemRestore")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/StartSystemRestore")))
				return soap_serve___tds__StartSystemRestore(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetStorageConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetStorageConfigurations")))
				return soap_serve___tds__GetStorageConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:CreateStorageConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/CreateStorageConfiguration")))
				return soap_serve___tds__CreateStorageConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetStorageConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetStorageConfiguration")))
				return soap_serve___tds__GetStorageConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetStorageConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetStorageConfiguration")))
				return soap_serve___tds__SetStorageConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:DeleteStorageConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/DeleteStorageConfiguration")))
				return soap_serve___tds__DeleteStorageConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:GetGeoLocation")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/GetGeoLocation")))
				return soap_serve___tds__GetGeoLocation(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:SetGeoLocation")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/SetGeoLocation")))
				return soap_serve___tds__SetGeoLocation(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tds:DeleteGeoLocation")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/device/wsdl/DeleteGeoLocation")))
				return soap_serve___tds__DeleteGeoLocation(soap);
		}
		
		if (strcmp(soap->path, "/onvif/media") == 0)
		{
			DBG_PRINT("+++ MEDIA +++");
			
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetServiceCapabilities")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetServiceCapabilities")))
				return soap_serve___trt__GetServiceCapabilities(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoSources")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdlGetVideoSources/")))
				return soap_serve___trt__GetVideoSources(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioSources")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioSources")))
				return soap_serve___trt__GetAudioSources(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioOutputs")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioOutputs")))
				return soap_serve___trt__GetAudioOutputs(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:CreateProfile")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/CreateProfile")))
				return soap_serve___trt__CreateProfile(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetProfile")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdlGetProfile/")))
				return soap_serve___trt__GetProfile(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetProfiles")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetProfiles")))
				return soap_serve___trt__GetProfiles(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddVideoEncoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddVideoEncoderConfiguration")))
				return soap_serve___trt__AddVideoEncoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddVideoSourceConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddVideoSourceConfiguration")))
				return soap_serve___trt__AddVideoSourceConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddAudioEncoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddAudioEncoderConfiguration")))
				return soap_serve___trt__AddAudioEncoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddAudioSourceConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddAudioSourceConfiguration")))
				return soap_serve___trt__AddAudioSourceConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddPTZConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddPTZConfiguration")))
				return soap_serve___trt__AddPTZConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddVideoAnalyticsConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddVideoAnalyticsConfiguration")))
				return soap_serve___trt__AddVideoAnalyticsConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddMetadataConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddMetadataConfiguration")))
				return soap_serve___trt__AddMetadataConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddAudioOutputConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddAudioOutputConfiguration")))
				return soap_serve___trt__AddAudioOutputConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:AddAudioDecoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/AddAudioDecoderConfiguration")))
				return soap_serve___trt__AddAudioDecoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemoveVideoEncoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemoveVideoEncoderConfiguration")))
				return soap_serve___trt__RemoveVideoEncoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemoveVideoSourceConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemoveVideoSourceConfiguration")))
				return soap_serve___trt__RemoveVideoSourceConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemoveAudioEncoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemoveAudioEncoderConfiguration")))
				return soap_serve___trt__RemoveAudioEncoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemoveAudioSourceConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemoveAudioSourceConfiguration")))
				return soap_serve___trt__RemoveAudioSourceConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemovePTZConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemovePTZConfiguration")))
				return soap_serve___trt__RemovePTZConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemoveVideoAnalyticsConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemoveVideoAnalyticsConfiguration")))
				return soap_serve___trt__RemoveVideoAnalyticsConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemoveMetadataConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemoveMetadataConfiguration")))
				return soap_serve___trt__RemoveMetadataConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemoveAudioOutputConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemoveAudioOutputConfiguration")))
				return soap_serve___trt__RemoveAudioOutputConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:RemoveAudioDecoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/RemoveAudioDecoderConfiguration")))
				return soap_serve___trt__RemoveAudioDecoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:DeleteProfile")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/DeleteProfile")))
				return soap_serve___trt__DeleteProfile(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoSourceConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfigurations")))
				return soap_serve___trt__GetVideoSourceConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoEncoderConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfigurations")))
				return soap_serve___trt__GetVideoEncoderConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioSourceConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdlGetAudioSourceConfigurations/")))
				return soap_serve___trt__GetAudioSourceConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioEncoderConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioEncoderConfigurations")))
				return soap_serve___trt__GetAudioEncoderConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoAnalyticsConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetVideoAnalyticsConfigurations")))
				return soap_serve___trt__GetVideoAnalyticsConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetMetadataConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetMetadataConfigurations")))
				return soap_serve___trt__GetMetadataConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioOutputConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioOutputConfigurations")))
				return soap_serve___trt__GetAudioOutputConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioDecoderConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioDecoderConfigurations")))
				return soap_serve___trt__GetAudioDecoderConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoSourceConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfiguration")))
				return soap_serve___trt__GetVideoSourceConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoEncoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfiguration")))
				return soap_serve___trt__GetVideoEncoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioSourceConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioSourceConfiguration")))
				return soap_serve___trt__GetAudioSourceConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioEncoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioEncoderConfiguration")))
				return soap_serve___trt__GetAudioEncoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoAnalyticsConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetVideoAnalyticsConfiguration")))
				return soap_serve___trt__GetVideoAnalyticsConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetMetadataConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetMetadataConfiguration")))
				return soap_serve___trt__GetMetadataConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioOutputConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioOutputConfiguration")))
				return soap_serve___trt__GetAudioOutputConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioDecoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioDecoderConfiguration")))
				return soap_serve___trt__GetAudioDecoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetCompatibleVideoEncoderConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetCompatibleVideoEncoderConfigurations")))
				return soap_serve___trt__GetCompatibleVideoEncoderConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetCompatibleVideoSourceConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetCompatibleVideoSourceConfigurations")))
				return soap_serve___trt__GetCompatibleVideoSourceConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetCompatibleAudioEncoderConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetCompatibleAudioEncoderConfigurations")))
				return soap_serve___trt__GetCompatibleAudioEncoderConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetCompatibleAudioSourceConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetCompatibleAudioSourceConfigurations")))
				return soap_serve___trt__GetCompatibleAudioSourceConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetCompatibleVideoAnalyticsConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetCompatibleVideoAnalyticsConfigurations")))
				return soap_serve___trt__GetCompatibleVideoAnalyticsConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetCompatibleMetadataConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetCompatibleMetadataConfigurations")))
				return soap_serve___trt__GetCompatibleMetadataConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetCompatibleAudioOutputConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetCompatibleAudioOutputConfigurations")))
				return soap_serve___trt__GetCompatibleAudioOutputConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetCompatibleAudioDecoderConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetCompatibleAudioDecoderConfigurations")))
				return soap_serve___trt__GetCompatibleAudioDecoderConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetVideoSourceConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetVideoSourceConfiguration")))
				return soap_serve___trt__SetVideoSourceConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetVideoEncoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetVideoEncoderConfiguration")))
				return soap_serve___trt__SetVideoEncoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetAudioSourceConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetAudioSourceConfiguration")))
				return soap_serve___trt__SetAudioSourceConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetAudioEncoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetAudioEncoderConfiguration")))
				return soap_serve___trt__SetAudioEncoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetVideoAnalyticsConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetVideoAnalyticsConfiguration")))
				return soap_serve___trt__SetVideoAnalyticsConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetMetadataConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetMetadataConfiguration")))
				return soap_serve___trt__SetMetadataConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetAudioOutputConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetAudioOutputConfiguration")))
				return soap_serve___trt__SetAudioOutputConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetAudioDecoderConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetAudioDecoderConfiguration")))
				return soap_serve___trt__SetAudioDecoderConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoSourceConfigurationOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdlGetVideoSourceConfigurationOptions/")))
				return soap_serve___trt__GetVideoSourceConfigurationOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoEncoderConfigurationOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfigurationOptions")))
				return soap_serve___trt__GetVideoEncoderConfigurationOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioSourceConfigurationOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioSourceConfigurationOptions")))
				return soap_serve___trt__GetAudioSourceConfigurationOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioEncoderConfigurationOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioEncoderConfigurationOptions")))
				return soap_serve___trt__GetAudioEncoderConfigurationOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetMetadataConfigurationOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetMetadataConfigurationOptions")))
				return soap_serve___trt__GetMetadataConfigurationOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioOutputConfigurationOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioOutputConfigurationOptions")))
				return soap_serve___trt__GetAudioOutputConfigurationOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetAudioDecoderConfigurationOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetAudioDecoderConfigurationOptions")))
				return soap_serve___trt__GetAudioDecoderConfigurationOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetGuaranteedNumberOfVideoEncoderInstances")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetGuaranteedNumberOfVideoEncoderInstances")))
				return soap_serve___trt__GetGuaranteedNumberOfVideoEncoderInstances(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetStreamUri")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetStreamUri")))
				return soap_serve___trt__GetStreamUri(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:StartMulticastStreaming")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/StartMulticastStreaming")))
				return soap_serve___trt__StartMulticastStreaming(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:StopMulticastStreaming")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/StopMulticastStreaming")))
				return soap_serve___trt__StopMulticastStreaming(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetSynchronizationPoint")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetSynchronizationPoint")))
				return soap_serve___trt__SetSynchronizationPoint(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetSnapshotUri")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetSnapshotUri")))
				return soap_serve___trt__GetSnapshotUri(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetVideoSourceModes")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetVideoSourceModes")))
				return soap_serve___trt__GetVideoSourceModes(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetVideoSourceMode")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetVideoSourceMode")))
				return soap_serve___trt__SetVideoSourceMode(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetOSDs")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetOSDs")))
				return soap_serve___trt__GetOSDs(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetOSD")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetOSD")))
				return soap_serve___trt__GetOSD(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:GetOSDOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/GetOSDOptions")))
				return soap_serve___trt__GetOSDOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:SetOSD")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/SetOSD")))
				return soap_serve___trt__SetOSD(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:CreateOSD")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/CreateOSD")))
				return soap_serve___trt__CreateOSD(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "trt:DeleteOSD")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/media/wsdl/DeleteOSD")))
				return soap_serve___trt__DeleteOSD(soap);
		}

		if (strcmp(soap->path, "/onvif/event") == 0)
		{
			DBG_PRINT("+++ EVENT +++");
			
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tev:CreatePullPointSubscription")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionRequest")))
				return soap_serve___tev__CreatePullPointSubscription(soap);
		}

		if (strcmp(soap->path, "/onvif/ptz") == 0)
		{
			DBG_PRINT("+++ PTZ +++");
			
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetServiceCapabilities")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetServiceCapabilities")))
				return soap_serve___tptz__GetServiceCapabilities(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetConfigurations")))
				return soap_serve___tptz__GetConfigurations(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetPresets")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetPresets")))
				return soap_serve___tptz__GetPresets(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:SetPreset")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/SetPreset")))
				return soap_serve___tptz__SetPreset(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:RemovePreset")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/RemovePreset")))
				return soap_serve___tptz__RemovePreset(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GotoPreset")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GotoPreset")))
				return soap_serve___tptz__GotoPreset(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetStatus")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetStatus")))
				return soap_serve___tptz__GetStatus(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetConfiguration")))
				return soap_serve___tptz__GetConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetNodes")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetNodes")))
				return soap_serve___tptz__GetNodes(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetNode")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetNode")))
				return soap_serve___tptz__GetNode(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:SetConfiguration")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/SetConfiguration")))
				return soap_serve___tptz__SetConfiguration(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetConfigurationOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetConfigurationOptions")))
				return soap_serve___tptz__GetConfigurationOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GotoHomePosition")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GotoHomePosition")))
				return soap_serve___tptz__GotoHomePosition(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:SetHomePosition")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/SetHomePosition")))
				return soap_serve___tptz__SetHomePosition(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:ContinuousMove")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/ContinuousMove")))
				return soap_serve___tptz__ContinuousMove(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:RelativeMove")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/RelativeMove")))
				return soap_serve___tptz__RelativeMove(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:SendAuxiliaryCommand")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/SendAuxiliaryCommand")))
				return soap_serve___tptz__SendAuxiliaryCommand(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:AbsoluteMove")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/AbsoluteMove")))
				return soap_serve___tptz__AbsoluteMove(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GeoMove")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GeoMove")))
				return soap_serve___tptz__GeoMove(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:Stop")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/Stop")))
				return soap_serve___tptz__Stop(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetPresetTours")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetPresetTours")))
				return soap_serve___tptz__GetPresetTours(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetPresetTour")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetPresetTour")))
				return soap_serve___tptz__GetPresetTour(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetPresetTourOptions")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetPresetTourOptions")))
				return soap_serve___tptz__GetPresetTourOptions(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:CreatePresetTour")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/CreatePresetTour")))
				return soap_serve___tptz__CreatePresetTour(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:ModifyPresetTour")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/ModifyPresetTour")))
				return soap_serve___tptz__ModifyPresetTour(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:OperatePresetTour")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/OperatePresetTour")))
				return soap_serve___tptz__OperatePresetTour(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:RemovePresetTour")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/RemovePresetTour")))
				return soap_serve___tptz__RemovePresetTour(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tptz:GetCompatibleConfigurations")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver20/ptz/wsdl/GetCompatibleConfigurations")))
				return soap_serve___tptz__GetCompatibleConfigurations(soap);
		}

		if (strncmp(soap->path, "/onvif/pullpoint/", strlen("/onvif/pullpoint/")) == 0)
		{
			DBG_PRINT("+++ EVENT +++");
			
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tev:PullMessages")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesRequest")))
				return soap_serve___tev__PullMessages(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tev:Seek")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/SeekRequest")))
				return soap_serve___tev__Seek(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tev:SetSynchronizationPoint")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/SetSynchronizationPointRequest")))
				return soap_serve___tev__SetSynchronizationPoint(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tev:GetServiceCapabilities")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/events/wsdl/EventPortType/GetServiceCapabilitiesRequest")))
				return soap_serve___tev__GetServiceCapabilities(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "tev:GetEventProperties")) || (soap->action && !strcmp(soap->action, "http://www.onvif.org/ver10/events/wsdl/EventPortType/GetEventPropertiesRequest")))
				return soap_serve___tev__GetEventProperties(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:Renew")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewRequest")))
				return soap_serve___tev__Renew(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:Unsubscribe")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/UnsubscribeRequest")))
				return soap_serve___tev__Unsubscribe(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:Subscribe")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/NotificationProducer/SubscribeRequest")))
				return soap_serve___tev__Subscribe(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:GetCurrentMessage")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/NotificationProducer/GetCurrentMessageRequest")))
				return soap_serve___tev__GetCurrentMessage(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:Notify")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify")))
				return soap_serve___tev__Notify(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:GetMessages")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/PullPoint/GetMessagesRequest")))
				return soap_serve___tev__GetMessages(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:DestroyPullPoint")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/PullPoint/DestroyPullPointRequest")))
				return soap_serve___tev__DestroyPullPoint(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:Notify")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/PullPoint/Notify")))
				return soap_serve___tev__Notify_(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:CreatePullPoint")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/CreatePullPoint/CreatePullPointRequest")))
				return soap_serve___tev__CreatePullPoint(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:Renew")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/PausableSubscriptionManager/RenewRequest")))
				return soap_serve___tev__Renew_(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:Unsubscribe")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/PausableSubscriptionManager/UnsubscribeRequest")))
				return soap_serve___tev__Unsubscribe_(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:PauseSubscription")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/PausableSubscriptionManager/PauseSubscriptionRequest")))
				return soap_serve___tev__PauseSubscription(soap);
			if ((soap->action == NULL && !soap_match_tag(soap, soap->tag, "wsnt:ResumeSubscription")) || (soap->action && !strcmp(soap->action, "http://docs.oasis-open.org/wsn/bw-2/PausableSubscriptionManager/ResumeSubscriptionRequest")))
				return soap_serve___tev__ResumeSubscription(soap);
		}
	}
	
	return soap->error = SOAP_NO_METHOD;
}

//------------------------------------------------------------------------------------------------------------------------------------------------
