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
#include "OnvifBridge.h"
#include "mhengine/MH_CriticalSection.h"
#include "mhengine/MH_Event.h"
#include "mhengine/MH_MessageQueueManager.h"
#include "mhmedia/MH_CameraDevice.h"
#include "MessageDef.h"
#include <unistd.h>
#include <uuid/uuid.h>
#include <map>
#include <string>
#include <vector>

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

//-------------------------------------------------------------------------------------------------------------------------

static SZ_OnvifProcess* g_process = NULL;

static MH_CriticalSection g_setting_sync;

static MH_CriticalSection g_motion_sync;

struct MOTION_EVENT
{
	MH_Event* notify_event;
	bool cancel;
	int waiting;
	timespec last_access;
};

static std::map<std::string, MOTION_EVENT> g_MotionDetectEvent;
static bool g_motion_detect = false;

//-------------------------------------------------------------------------------------------------------------------------

/**
 * ブリッジ関数初期化
 */
bool OnvifBridgeInitialize(SZ_OnvifProcess* process)
{
	DBG_TRACE("Enter OnvifBridgeInitialize");
	
	g_process       = process;
	g_motion_detect = false;
	
	DBG_TRACE("Exit OnvifBridgeInitialize");
	
	return true;
}

/**
 * ブリッジ関数終了
 */
void OnvifBridgeTerminate()
{
	DBG_TRACE("Enter OnvifBridgeTerminate");

	for(std::map<std::string, MOTION_EVENT>::iterator it = g_MotionDetectEvent.begin(); it != g_MotionDetectEvent.end(); it++)
	{
		MOTION_EVENT& motionEvent = it->second;
		motionEvent.notify_event->Close();
		delete motionEvent.notify_event;
	}
	g_MotionDetectEvent.clear();

	DBG_TRACE("Exit OnvifBridgeTerminate");
}

//-------------------------------------------------------------------------------------------------------------------------

/**
 * イベント通知
 */
void SetMotionEvent(bool motion_detect)
{
	DBG_TRACE("Enter SetMotionEvent");
	DBG_PRINT("motion_detect = %d", motion_detect);
	
	{
		MH_AutoLock lock(&g_motion_sync);
	
		g_motion_detect = motion_detect;
		
		for(std::map<std::string, MOTION_EVENT>::iterator it = g_MotionDetectEvent.begin(); it != g_MotionDetectEvent.end(); it++)
		{
			MOTION_EVENT& motionEvent = it->second;
			motionEvent.notify_event->Set();
		}
	}
	
	DBG_TRACE("Exit SetMotionEvent");
}

extern "C" {

//-------------------------------------------------------------------------------------------------------------------------

/**
 * ユーザー数を取得する
 */
int Bridge_GetUserCount()
{
	SZ_UserSetting* user_setting = g_process->GetOnvifPlatform()->GetUserSetting();
	return user_setting->GetUserCount();
}

/**
 * ユーザ情報を取得する
 */
int Bridge_GetUserInfo(int index, struct ONVIF_USER_INFO* user_info)
{
	SZ_UserSetting* user_setting = g_process->GetOnvifPlatform()->GetUserSetting();
	if (!user_setting->GetUserInfo(index, user_info))
	{
		return -1;
	}
	return 0;
}

/**
 * ユーザ情報を検索する
 */
int Bridge_FindUserInfo(const char* username, struct ONVIF_USER_INFO* user_info)
{
	SZ_UserSetting* user_setting = g_process->GetOnvifPlatform()->GetUserSetting();
	if (!user_setting->FindUserInfo(username, user_info))
	{
		return -1;
	}
	return 0;
}

/**
 * ユーザ情報を追加する
 */
void Bridge_AddUserInfo(const struct ONVIF_USER_INFO* user_info)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->AddUserInfo(user_info);
}

/**
 * ユーザ情報を更新する
 */
void Bridge_SetUserInfo(const struct ONVIF_USER_INFO* user_info)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->SetUserInfo(user_info);
}

/**
 * ユーザ情報を削除する
 */
void Bridge_DeleteUserInfo(const char* username)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->DeleteUserInfo(username);
}

//-------------------------------------------------------------------------------------------------------------------------

/**
 * デバイス情報を通知する
 */
void Bridge_GetDeviceInfo(ONVIF_DEVICE_INFO* device_info)
{
	g_process->GetOnvifPlatform()->GetDeviceInfo(device_info);
}

//-------------------------------------------------------------------------------------------------------------------------

/**
 * デバイスIDを取得する
 */
void Bridge_GetOnvifIdntification(char* name, char* location)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->GetOnvifId(name, location);
}

/**
 * デバイスIDを設定する
 */
void Bridge_SetOnvifIdntification(const char* name, const char* location)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->SetOnvifId(name, location);
}

//-------------------------------------------------------------------------------------------------------------------------

/**
 * ディスカバリー機能の状態を取得する
 */
int Bridge_GetDiscoveryMode()
{
	MH_AutoLock lock(&g_setting_sync);
	
	if (!g_process->GetDiscoveryMode())
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/**
 * ディスカバリー機能の状態を設定する
 */
void Bridge_SetDiscoveryMode(int discovery_mode)
{
	MH_AutoLock lock(&g_setting_sync);
	
	if (discovery_mode == 0)
	{
		g_process->SetDiscoveryMode(false);
	}
	else
	{
		g_process->SetDiscoveryMode(true);
	}
}

//-------------------------------------------------------------------------------------------------------------------------

void Bridge_SetHostname(const char* hostname)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->GetOnvifPlatform()->GetNetworkSetting()->SetHostname(hostname);
}

void Bridge_GetHostname(char* hostname)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->GetOnvifPlatform()->GetNetworkSetting()->GetHostname(hostname);
}

//-------------------------------------------------------------------------------------------------------------------------

int Bridge_GetOnvifPortNo()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetOnvifPortNo();
}

//-------------------------------------------------------------------------------------------------------------------------

int Bridge_GetSnapShotPortNo()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetSnapShotPortNo();
}

int Bridge_SetSnapShotPortNo(int port_no)
{
	return g_process->SetSnapShotPortNo(port_no);
}

int Bridge_GetRtspPortNo()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetRtspPortNo();
}

int Bridge_SetRtspPortNo(int port_no)
{
	return g_process->SetRtspPortNo(port_no);
}

void Bridge_GetDnsAddress(int* use_dhcp, char* dns_address)
{
	MH_AutoLock lock(&g_setting_sync);
	bool use_dhcp_temp = 0;
	g_process->GetOnvifPlatform()->GetNetworkSetting()->GetDnsAddress(&use_dhcp_temp, dns_address);
	
	if (!use_dhcp_temp)
	{
		*use_dhcp = 0;
	}
	else
	{
		*use_dhcp = 1;
	}
}

int Bridge_SetDnsAddress(int use_dhcp, const char* dns_address)
{
	MH_AutoLock lock(&g_setting_sync);
	
	bool ret;
	if (use_dhcp == 0)
	{
		ret = g_process->GetOnvifPlatform()->GetNetworkSetting()->SetDnsAddress(false, dns_address);
	}
	else
	{
		ret = g_process->GetOnvifPlatform()->GetNetworkSetting()->SetDnsAddress(true, dns_address);
	}
	if (!ret)
	{
		return -1;
	}
	
	return 0;
}

void Bridge_GetGatewayAddress(char* address)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->GetOnvifPlatform()->GetNetworkSetting()->GetGatewayAddress(address);
}

int Bridge_SetGatewayAddress(const char* address)
{
	MH_AutoLock lock(&g_setting_sync);
	
	if (!g_process->GetOnvifPlatform()->GetNetworkSetting()->SetGatewayAddress( address))
	{
		return -1;
	}
	
	return 0;
}

void Bridge_GetNetwork(char* if_name, char* hw_address, int* use_dhcp, char* ip_address, int* prefix_len)
{
	MH_AutoLock lock(&g_setting_sync);
	
	bool use_dhcp_temp = 0;
	g_process->GetOnvifPlatform()->GetNetworkSetting()->GetNetwork(if_name, hw_address, &use_dhcp_temp, ip_address, prefix_len);
	
	if (!use_dhcp_temp)
	{
		*use_dhcp = 0;
	}
	else
	{
		*use_dhcp = 1;
	}
}

int Bridge_SetNetwork(const char* if_name, int use_dhcp, const char* ip_address, int prefix_len)
{
	MH_AutoLock lock(&g_setting_sync);
	
	bool ret;
	if (use_dhcp == 0)
	{
		ret = g_process->GetOnvifPlatform()->GetNetworkSetting()->SetNetwork(if_name, false, ip_address, prefix_len);
	}
	else
	{
		ret = g_process->GetOnvifPlatform()->GetNetworkSetting()->SetNetwork(if_name, true, ip_address, prefix_len);
	}
	if (!ret)
	{
		return -1;
	}
	
	return 0;
}

const char* Bridge_GetIpAddres()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetNetworkSetting()->GetAddress();
}

//-------------------------------------------------------------------------------------------------------------------------

/**
 * ファクトリリセットを行う
 */
void Bridge_FactoryReset(int type)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->FactoryReset(type);
}

/**
 * リブートを行う
 */
void Bridge_SystemReboot()
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->SystemReboot();
}

//-------------------------------------------------------------------------------------------------------------------------

int Bridge_GetVideoSourceCount()
{
	return 1;
}

int Bridge_GetVideoSource(int index, struct ONVIF_VIDEO_SOURCE_INFO* video_source_info)
{
	DBG_TRACE("Enter Bridge_GetVideoSource");
	
	MH_CameraDevice* camera = g_process->GetCameraDevice();
	camera->GetHardwareId(video_source_info->token);
	
	video_source_info->framerate = camera->GetMaxFramerate();
	video_source_info->width     = camera->GetMaxWidth();
	video_source_info->height    = camera->GetMaxHeight();
	
	DBG_TRACE("Exit Bridge_GetVideoSource");
	
	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------

int Bridge_GetVideoSourceConfigCount()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetProfileSetting()->GetVideoSourceConfigCount();
}

int Bridge_GetVideoSourceConfig(int index, struct ONVIF_VIDEO_SOURCE_CONFIG_INFO* video_source_config_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetOnvifPlatform()->GetProfileSetting()->GetVideoSourceConfig(index, video_source_config_info))
	{
		return -1;
	}
	return 0;
}

int Bridge_FindVideoSourceConfig(const char* token, struct ONVIF_VIDEO_SOURCE_CONFIG_INFO* video_source_config_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetOnvifPlatform()->GetProfileSetting()->FindVideoSourceConfig(token, video_source_config_info))
	{
		return -1;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------

int Bridge_GetVideoEncoderConfigCount()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetProfileSetting()->GetVideoEncoderConfigCount();
}

int Bridge_GetCompatibleVideoEncoderConfigCount(int width, int height)
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetProfileSetting()->GetCompatibleVideoEncoderConfigCount(width, height);
}

int Bridge_GetVideoEncoderConfig(int index, struct ONVIF_VIDEO_ENCODER_CONFIG_INFO* video_encoder_config_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetOnvifPlatform()->GetProfileSetting()->GetVideoEncoderConfig(index, video_encoder_config_info))
	{
		return -1;
	}
	return 0;
}

int Bridge_FindVideoEncoderConfig(const char* token, struct ONVIF_VIDEO_ENCODER_CONFIG_INFO* video_encoder_config_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetOnvifPlatform()->GetProfileSetting()->FindVideoEncoderConfig(token, video_encoder_config_info))
	{
		return -1;
	}
	return 0;
}

int Bridge_GetSupportedResolutionCount()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetProfileSetting()->GetSupportedResolutionCount();
}

void Bridge_GetSupportedResolution(int index, int* width, int* height)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->GetOnvifPlatform()->GetProfileSetting()->GetSupportedResolution(index, width, height);
}

int Bridge_ChangeVideoEncoderConfig(const char* token, int quality)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->ChangeVideoEncoderConfig(token, quality))
	{
		return -1;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------

int Bridge_GetPtzConfigCount()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetProfileSetting()->GetPtzConfigCount();
}

int Bridge_GetPtzConfig(int index, struct ONVIF_PTZ_CONFIG_INFO* ptz_config_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetOnvifPlatform()->GetProfileSetting()->GetPtzConfig(index, ptz_config_info))
	{
		return -1;
	}
	return 0;
}

int Bridge_FindPtzConfig(const char* ptz_token, struct ONVIF_PTZ_CONFIG_INFO* ptz_config_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetOnvifPlatform()->GetProfileSetting()->FindPtzConfig(ptz_token, ptz_config_info))
	{
		return -1;
	}
	return 0;
}

int Bridge_GetPtzStatus(const char* profile_token, float* x, float* y, int* is_moving)
{
	MH_AutoLock lock(&g_setting_sync);
	SZ_PtzDevice* ptz = g_process->GetPtzDevice(profile_token);
	if (ptz == NULL)
	{
		return -1;
	}
	
	*x = ptz->GetPosX();
	*y = ptz->GetPosY();
	
	if (ptz->IsMoving())
	{
		*is_moving = 1;
	}
	else
	{
		*is_moving = 0;
	}
	
	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------

/**
 * プロファイル数取得
 */
int Bridge_GetProfileCount()
{
	MH_AutoLock lock(&g_setting_sync);
	return g_process->GetOnvifPlatform()->GetProfileSetting()->GetProfileCount();
}

/**
 * プロファイル取得
 */
int Bridge_GetProfile(int index, struct ONVIF_PROFILE_INFO* profile_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetOnvifPlatform()->GetProfileSetting()->GetProfile(index, profile_info))
	{
		return -1;
	}
	return 0;
}

/**
 * プロファイル検索
 */
int Bridge_FindProfile(const char* token, struct ONVIF_PROFILE_INFO* profile_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetOnvifPlatform()->GetProfileSetting()->FindProfile(token, profile_info))
	{
		return -1;
	}
	return 0;
}

/**
 * プロファイル作成
 */
int Bridge_CreateProfile(const char* name, const char* token, struct ONVIF_PROFILE_INFO* profile_info)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->CreateProfile(name, token, profile_info))
	{
		return -1;
	}
	return 0;
}

/**
 * プロファイル削除
 */
int Bridge_DeleteProfile(const char* token)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->DeleteProfile(token))
	{
		return -1;
	}
	return 0;
}

/**
 * ビデオソース追加
 */
int Bridge_AddVideoSource(const char* profile_token, const char* source_token)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->AddVideoSource(profile_token, source_token))
	{
		return -1;
	}
	return 0;
}

/**
 * ビデオソース削除
 */
int Bridge_RemoveVideoSource(const char* profile_token)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->RemoveVideoSource(profile_token))
	{
		return -1;
	}
	return 0;
}

/**
 * ビデオエンコーダー追加
 */
int Bridge_AddVideoEncoder(const char* profile_token, const char* encoder_token)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->AddVideoEncoder(profile_token, encoder_token))
	{
		return -1;
	}
	return 0;
}

/**
 * ビデオエンコーダー削除
 */
int Bridge_RemoveVideoEncoder(const char* profile_token)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->RemoveVideoEncoder(profile_token))
	{
		return -1;
	}
	return 0;
}

int Bridge_AddPtz(const char* profile_token, const char* ptz_token)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->AddPtz(profile_token, ptz_token))
	{
		return -1;
	}
	return 0;
}

int Bridge_RemovePtz(const char* profile_token)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->RemovePtz(profile_token))
	{
		return -1;
	}
	return 0;
}

int Bridge_UpdateHomePosition(const char* profile_token, float x, float y)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->UpdateHomePosition(profile_token, x, y))
	{
		return -1;
	}
	return 0;
}

/**
 * スナップショットURI取得
 */
int Bridge_GetSnapShotUri(const char* profile_token, char* uri)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetSnapShotUri(profile_token, uri))
	{
		return -1;
	}
	return 0;
}

/**
 * ストリーミングURI取得
 */
int Bridge_GetRtspUri(const char* profile_token, char* uri)
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetRtspUri(profile_token, uri))
	{
		return -1;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------

int Bridge_CreateMotionEvent(char* pull_point)
{
	DBG_TRACE("Enter Bridge_CreateMotionEvent");
	
	{
		MH_AutoLock lock(&g_motion_sync);
		
		uuid_t uuid;
		uuid_generate(uuid);
		uuid_unparse(uuid, pull_point);
		DBG_PRINT("pull_point = %s", pull_point);
		
		MOTION_EVENT motion_event;
		motion_event.cancel = false;
		motion_event.waiting = 0;
		motion_event.notify_event = new MH_Event();
		motion_event.notify_event->Create();
		clock_gettime(CLOCK_MONOTONIC, &motion_event.last_access);

		g_MotionDetectEvent[pull_point] = motion_event;
	}
	
	DBG_TRACE("Exit Bridge_CreateMotionEvent");

	return 0;
}

void Bridge_DeleteMotionEvent(const char* pull_point)
{
	DBG_TRACE("Enter Bridge_DeleteMotionEvent");
	DBG_PRINT("pull_point = %s", pull_point);
	
	g_motion_sync.Lock();
	
	std::map<std::string, MOTION_EVENT>::iterator it = g_MotionDetectEvent.find(pull_point);
	if (it == g_MotionDetectEvent.end())
	{
		g_motion_sync.Unlock();
		DBG_PRINT("Error: NotFound");
		return;
	}
	
	MOTION_EVENT& motionEvent = it->second;
	motionEvent.cancel = true;
	
	if (motionEvent.waiting > 0)
	{
		motionEvent.notify_event->Set();
		
		g_motion_sync.Unlock();
		
		while(motionEvent.waiting > 0)
		{
			usleep(1000);
		}
		
		g_motion_sync.Lock();
	}
	
	motionEvent.notify_event->Close();
	delete motionEvent.notify_event;
	
	g_MotionDetectEvent.erase(it);

	g_motion_sync.Unlock();
	
	DBG_TRACE("Exit Bridge_DeleteMotionEvent");
}

int Bridge_RenewMotionEvent(const char* pull_point)
{
	DBG_TRACE("Enter Bridge_RenewMotionEvent");
	DBG_PRINT("pull_point = %s", pull_point);
	
	{
		MH_AutoLock lock(&g_motion_sync);
		
		std::map<std::string, MOTION_EVENT>::iterator it = g_MotionDetectEvent.find(pull_point);
		if (it == g_MotionDetectEvent.end())
		{
			DBG_PRINT("Error: NotFound");
			return -1;
		}

		MOTION_EVENT& motionEvent = it->second;
		
		clock_gettime(CLOCK_MONOTONIC, &motionEvent.last_access);
	}
	
	DBG_TRACE("Exit Bridge_RenewMotionEvent");
	
	return 0;
}

int Bridge_WaitMotionEvent(const char* pull_point, int timeout, char* video_source_token, int* motion_detect)
{
	DBG_TRACE("Enter Bridge_WaitMotionEvent");
	DBG_PRINT("pull_point = %s", pull_point);
	DBG_PRINT("timeout = %d\n", timeout);
	
	g_motion_sync.Lock();
	
	std::map<std::string, MOTION_EVENT>::iterator it = g_MotionDetectEvent.find(pull_point);
	if (it == g_MotionDetectEvent.end())
	{
		g_motion_sync.Unlock();
		
		DBG_PRINT("Error: NotFound");
		return -1;
	}
	
	MOTION_EVENT& motionEvent = it->second;
	
	if (motionEvent.cancel)
	{
		g_motion_sync.Unlock();
		
		DBG_PRINT("Info: Cancel");
		return -1;
	}
	
	motionEvent.waiting ++;

	g_motion_sync.Unlock();
	
	unsigned long t = (unsigned long)timeout;
	motionEvent.notify_event->Wait(t);

	g_motion_sync.Lock();

	motionEvent.waiting --;

	if (motionEvent.cancel)
	{
		g_motion_sync.Unlock();
		
		DBG_PRINT("Info: Cancel");
		return -1;
	}

	MH_CameraDevice* camera = g_process->GetCameraDevice();
	camera->GetHardwareId(video_source_token);

	if (g_motion_detect)
	{
		*motion_detect = 1;
	}
	else
	{
		*motion_detect = 0;
	}
	
	clock_gettime(CLOCK_MONOTONIC, &motionEvent.last_access);
	
	g_motion_sync.Unlock();
	
	DBG_TRACE("Exit Bridge_WaitMotionEvent");

	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------

void Bridge_SetRecordingMode(int mode)
{
	MH_AutoLock lock(&g_setting_sync);
	if (mode == 0)
	{
		g_process->SetRecordingMode(false);
	}
	else
	{
		g_process->SetRecordingMode(true);
	}
}

int Bridge_GetRecordingMode()
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->GetRecordingMode())
	{
		return -1;
	}
	return 0;
}

int Bridge_StartRecording()
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->StartRecording())
	{
		return -1;
	}
	return 0;
}

int Bridge_StopRecording()
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->StopRecording())
	{
		return -1;
	}
	return 0;
}

int Bridge_IsRecording()
{
	MH_AutoLock lock(&g_setting_sync);
	if (!g_process->IsRecording())
	{
		return 1;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------

int Bridge_IsSupportedPTZ()
{
	if (g_process->IsSupportedPTZ())
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void Bridge_AbsoluteMove(const char* profile_token, float x, float y, float sx, float sy)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->AbsoluteMove(profile_token, x, y, sx, sy);
}

void Bridge_RelativeMove(const char* profile_token, float x, float y, float sx, float sy)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->RelativeMove(profile_token, x, y, sx, sy);
}

void Bridge_ContinuousMove(const char* profile_token, float sx, float sy)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->ContinuousMove(profile_token, sx, sy);
}

void Brdige_StopMove(const char* profile_token)
{
	MH_AutoLock lock(&g_setting_sync);
	g_process->StopMove(profile_token);
}

//-------------------------------------------------------------------------------------------------------------------------

}
