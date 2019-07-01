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
#include "mhonvif/SZ_OnvifProcess.h"
#include "OnvifService.h"
#include "SnapShotWebServer.h"
#include "ImageProcessThread.h"
#include "OnvifBridge.h"
#include "MessageDef.h"

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

//-------------------------------------------------------------------------------

/**
 * コンストラクタ
 */
SZ_OnvifProcess::SZ_OnvifProcess(SZ_OnvifPlatform* setting)
	: MH_MessageTask(QUEU_ID_ONVIF_PROCESS)
	, m_platform(setting)
	, m_recoder(NULL)
	, m_snapshot_server(NULL)
	, m_onvif_service(NULL)
	, m_discovery_server(NULL)
{
	DBG_TRACE("Enter SZ_OnvifProcess::SZ_OnvifProcess");
	
	DBG_TRACE("Exit SZ_OnvifProcess::SZ_OnvifProcess");
}

/**
 * デストラクタ
 */
SZ_OnvifProcess::~SZ_OnvifProcess()
{
	DBG_TRACE("Enter SZ_OnvifProcess::~SZ_OnvifProcess");
	
	DBG_TRACE("Exit SZ_OnvifProcess::~SZ_OnvifProcess");
}

/**
 * 設定取得
 */
SZ_OnvifPlatform* SZ_OnvifProcess::GetOnvifPlatform()
{
	return m_platform;
}

MH_CameraDevice* SZ_OnvifProcess::GetCameraDevice()
{
	return m_platform->GetCameraDevice();
}

SZ_PtzDevice* SZ_OnvifProcess::GetPtzDevice(const char* profile_token)
{
	ONVIF_PROFILE_INFO profile_info;
	if (!m_platform->GetProfileSetting()->FindProfile(profile_token, &profile_info))
	{
		return NULL;
	}
	return m_platform->GetPtzDevice(profile_info.ptz_token);
}

//-------------------------------------------------------------------------------

/**
 * タスク初期化
 */
bool SZ_OnvifProcess::OnTaskInitialize()
{
	DBG_TRACE("Enter SZ_OnvifProcess::OnTaskInitialize");

	//--------------------------------------------------
	// カメラを初期化する
	//--------------------------------------------------
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	// RTSP/サーバー/画像処理スレッドの配列を用意しておく
	int capture_num = camera->GetCaptureNum();
	if (capture_num == 0)
	{
		DBG_PRINT("Error: GetCaptureNum = 0");
		return false;
	}
	m_rtsp_server.resize(capture_num, NULL);
	m_image_process.resize(capture_num, NULL);
	
	//--------------------------------
	// サービスを起動する
	//--------------------------------
	
	if (!StartService())
	{
		return false;
	}

	DBG_TRACE("Exit SZ_OnvifProcess::OnTaskInitialize");
	
	return true;
}

/**
 * タスク終了
 */
void SZ_OnvifProcess::OnTaskTerminate()
{
	DBG_TRACE("Enter SZ_OnvifProcess::OnTaskTerminate");
	
	//--------------------------------
	// サービスを停止する
	//--------------------------------
	
	StopService();
	
	//--------------------------------------------------
	// カメラを終了する
	//--------------------------------------------------
	
	m_rtsp_server.clear();
	m_image_process.clear();
	
	DBG_TRACE("Exit SZ_OnvifProcess::OnTaskTerminate");
}

//-------------------------------------------------------------------------------

/**
 * サービス起動
 */
bool SZ_OnvifProcess::StartService()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StartService");

	//---------------------------------------------
	// ネットワークの設定を取得する
	//---------------------------------------------

	while(!IsTerminate())
	{
		if (m_platform->GetNetworkSetting()->Load())
		{
			break;
		}
		
		ThreadSleep(1000);
	}
	
	if (IsTerminate())
	{
		return false;
	}
	
	//---------------------------------------------
	// スクリーンショットサーバーを起動する
	//---------------------------------------------

	m_snapshot_server = new SnapShotWebServer(m_platform->GetSnapShotFilePath());
	
	if (!StartSnapShotServer())
	{
		StopService();
		return false;
	}
	
	//--------------------------------------------------
	// プロファイルの数だけカメラキャプチャを開始する
	//--------------------------------------------------

	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	int count = profile_setting->GetProfileCount();
	for(int i = 0; i < count; i++)
	{
		struct ONVIF_PROFILE_INFO profile_info;
		profile_setting->GetProfile(i, &profile_info);
		
		DBG_PRINT("[%d] name  = %s", i, profile_info.name);
		DBG_PRINT("[%d] token = %s", i, profile_info.token);
		
		MH_CaptureDevice* capture = camera->ReserveCaptureDevice();
		if (capture == NULL)
		{
			StopService();
			return false;
		}
		profile_setting->SetCaptureDeviceId(profile_info.token, capture->GetDeviceId());
		
		int index = camera->GetCaptureIndex(capture->GetDeviceId());
		if (!StartCameraDevice(index, &profile_info))
		{
			StopService();
			return false;
		}
		
		if (m_platform->GetRecordingMode())
		{
			StartRecording();
		}
	}
	
	//--------------------------------------------------
	// ONVIFサービスを起動する
	//--------------------------------------------------
	
	if (!StartOnvifService())
	{
		StopService();
		return false;
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::StartService");

	return true;
}

/**
 * サービス終了
 */
void SZ_OnvifProcess::StopService()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StopService");
	
	//--------------------------------------------------
	// ONVIFサービスを停止する
	//--------------------------------------------------
	
	StopOnvifService();
	
	//--------------------------------------------------
	// カメラキャプチャを停止する
	//--------------------------------------------------
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	int count = profile_setting->GetProfileCount();
	DBG_PRINT("GetProfileCount = %d", count);
	
	for(int i = 0; i < count; i++)
	{
		struct ONVIF_PROFILE_INFO profile_info;
		profile_setting->GetProfile(i, &profile_info);
		
		int index = camera->GetCaptureIndex(profile_info.capture_device_id);
		if (index >= 0)
		{
			StopRecording();
			
			StopCameraDevice(index, &profile_info);
		
			MH_CaptureDevice* capture = camera->GetCaptureDevice(index);
			if (capture != NULL)
			{
				return;
			}
			
			camera->ReleaseCaptureDevice(capture);
		}
	}
	
	StopSnapShotServer();
	
	if (m_snapshot_server != NULL)
	{
		delete m_snapshot_server;
		m_snapshot_server = NULL;
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::StopService");
}

//-------------------------------------------------------------------------------

/**
 * カメラキャプチャを開始する
 */
bool SZ_OnvifProcess::StartCameraDevice(int index, const ONVIF_PROFILE_INFO* profile_info)
{
	DBG_TRACE("Enter SZ_OnvifProcess::StartCameraDevice");
	DBG_PRINT("index = %d", index);

	//---------------------------------------------
	// カメラキャプチャを開始する
	//---------------------------------------------
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	MH_CaptureDevice* capture = camera->GetCaptureDevice(index);
	if (capture == NULL)
	{
		DBG_PRINT("Error: GetCaptureDevice = NULL");
		return false;
	}
	
	//---------------------------------------------
	// 設定を取得する
	//---------------------------------------------

	struct ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
	if (!m_platform->GetProfileSetting()->FindVideoEncoderConfig(profile_info->video_encoder_token, &video_encoder_config_info))
	{
		// キャプチャ起動対象外
		DBG_PRINT("Warnning: Not Found FindVideoEncoderConfig");
		return true;
	}
	
	if (m_recoder != NULL)
	{
		capture->AddSink(m_recoder);
		m_recoder->Start(
			capture->GetWidth(), 
			capture->GetHeight(), 
			m_platform->GetRecordingPath(),
			m_platform->GetRecordingInterval(),
			m_platform->GetRecordingFileNum(),
			m_platform->GetRecordingKeepSize()
		);
	}
	
	CaptureParam param;

	param.width     = video_encoder_config_info.width;
	param.height    = video_encoder_config_info.height;
	param.framerate = 60;
	param.bitrate   = 6000000;
	
	if (!capture->Start(&param))
	{
		StopCameraDevice(index, profile_info);
		return false;
	}

	//---------------------------------------------
	// RTSPサーバーを起動する
	//---------------------------------------------
	
	if (!StartStreaming(index, capture, profile_info))
	{
		StopCameraDevice(index, profile_info);
		return false;
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::StartCameraDevice");
	
	return true;
}

/**
 * カメラキャプチャを停止する
 */
void SZ_OnvifProcess::StopCameraDevice(int index, const ONVIF_PROFILE_INFO* profile_info)
{
	DBG_TRACE("Enter SZ_OnvifProcess::StopCameraDevice");
	
	StopStreaming(index, profile_info);
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	MH_CaptureDevice* capture = camera->GetCaptureDevice(index);
	if (capture == NULL)
	{
		return;
	}
	
	if (m_recoder != NULL)
	{
		capture->RemoveSink(m_recoder);
		m_recoder->Stop();
	}
	
	capture->Stop();

	DBG_TRACE("Exit SZ_OnvifProcess::StopCameraDevice");
}

/**
 * カメラキャプチャを再起動する
 */
bool SZ_OnvifProcess::RestartCameraDevice(const ONVIF_PROFILE_INFO* profile_info)
{
	DBG_TRACE("Enter SZ_OnvifProcess::RestartCameraDevice");
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	int index = camera->GetCaptureIndex(profile_info->capture_device_id);
	if (index >= 0)
	{
		StopCameraDevice(index, profile_info);
		StartCameraDevice(index, profile_info);
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::RestartCameraDevice");
	return true;
}

//-------------------------------------------------------------------------------

/**
 * 動画配信を開始する
 */
bool SZ_OnvifProcess::StartStreaming(int index, MH_CaptureDevice* capture, const ONVIF_PROFILE_INFO* profile_info)
{
	DBG_TRACE("Enter SZ_OnvifProcess::StartStreaming");
	
	if (m_rtsp_server[index] != NULL)
	{
		// 起動済み
		return true;
	}
	
	//---------------------------------------------
	// RTSPサーバーを起動する
	//---------------------------------------------
	
	m_rtsp_server[index] = new MH_RtspServerThread();
	m_rtsp_server[index]->Create();
	
	SZ_UserSetting* user_setting = m_platform->GetUserSetting();
	
	// ユーザを追加する
	int cnt = user_setting->GetUserCount();
	for (int i = 0; i < cnt; i++)
	{
		ONVIF_USER_INFO user_info;
		user_setting->GetUserInfo(i, &user_info);
		m_rtsp_server[index]->AddUser(user_info.username, user_info.password);
	}
	
	DBG_PRINT("RTSP: port_no = %d", m_platform->GetRtspPortNo());
	
	// サーバーを起動する
	if (!m_rtsp_server[index]->Start(capture, capture->GetDeviceId(), m_platform->GetRtspPortNo(), m_platform->GetRtspBufferingSec(), true))
	{
		StopStreaming(index, profile_info);
		return false;
	}
	
	//---------------------------------------------
	// 静止画処理スレッドを開始する
	//---------------------------------------------
	
	m_snapshot_server->AddProfile(profile_info->token);
	
	int width  = capture->GetWidth();
	int height = capture->GetHeight();

	m_image_process[index] = new ImageProcessThread(capture, m_snapshot_server);
	if (!m_image_process[index]->Start(profile_info->token, width, height, m_platform->GetSnapShotWidth()))
	{
		DBG_PRINT("Error: ImageProcessThread::Start");
		StopStreaming(index, profile_info);
		return false;
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::StartStreaming");
	
	return true;
}

/**
 * 動画配信を停止する
 */
void SZ_OnvifProcess::StopStreaming(int index, const ONVIF_PROFILE_INFO* profile_info)
{
	DBG_TRACE("Enter SZ_OnvifProcess::StopStreaming");
	
	if (m_rtsp_server[index] != NULL)
	{
		m_rtsp_server[index]->Stop();
		m_rtsp_server[index]->Destroy();
		delete m_rtsp_server[index];
		m_rtsp_server[index] = NULL;
	}
	
	if (m_image_process[index] != NULL)
	{
		m_image_process[index]->Stop();
		delete m_image_process[index];
		m_image_process[index] = NULL;
	}
	
	m_snapshot_server->RemoveProfile(profile_info->token);
	
	DBG_TRACE("Exit SZ_OnvifProcess::StopStreaming");
}

//-------------------------------------------------------------------------------

/**
 * 静止画配信を開始する
 */
bool SZ_OnvifProcess::StartSnapShotServer()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StartSnapShotServer");

	//---------------------------------------------
	// スナップショット配信用のWEBサーバーを開始する
	//---------------------------------------------
	
	DBG_PRINT("HTTP: port_no = %d", m_platform->GetSnapShotPortNo());
	
	if (!m_snapshot_server->Start(m_platform->GetSnapShotPortNo(), true))
	{
		DBG_PRINT("Error: SnapShotWebServer::Start");
		StopSnapShotServer();
		return false;
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::StartSnapShotServer");
	
	return true;
}

/**
 * 静止画配信を停止する
 */
void SZ_OnvifProcess::StopSnapShotServer()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StopSnapShotServer");
	
	if (m_snapshot_server != NULL)
	{
		m_snapshot_server->Stop();
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::StopSnapShotServer");
}

//-------------------------------------------------------------------------------

/**
 * ONVIF I/F を起動する
 */
bool SZ_OnvifProcess::StartOnvifService()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StartOnvifService");
	
	if (m_onvif_service != NULL)
	{
		// 起動済み
		return true;
	}
	
	OnvifBridgeInitialize(this);
	
	//--------------------------------
	// ONVIFサービスを起動する
	//--------------------------------
	
	m_onvif_service = new OnvifService();
	m_onvif_service->Start(
		m_platform->GetOnvifPortNo()
	);

	//--------------------------------
	// ディスカバリーサービスを起動する
	//--------------------------------
	
	StartDiscovery();
	
	DBG_TRACE("Exit SZ_OnvifProcess::StartOnvifService");
	
	return true;
}

/**
 * ONVIF I/F を停止する
 */
void SZ_OnvifProcess::StopOnvifService()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StopOnvifService");
	
	StopDiscovery();
	
	if (m_onvif_service != NULL)
	{
		m_onvif_service->Stop();
		delete m_onvif_service;
		m_onvif_service = NULL;
	}
	
	OnvifBridgeTerminate();
	
	DBG_TRACE("Exit SZ_OnvifProcess::StopOnvifService");
}

//-------------------------------------------------------------------------------

/**
 * Discoveryサービスを起動する
 */
bool SZ_OnvifProcess::StartDiscovery()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StartDiscovery");
	
	if (!m_platform->GetDiscoveryEnable())
	{
		// 無効
		return true;
	}
	
	if (m_discovery_server != NULL)
	{
		// 起動済み
		return true;
	}
	
	//--------------------------------
	// ディスカバリーサービスを起動する
	//--------------------------------
	
	char xaddr[255];
	sprintf(xaddr, "http://%s:%d/onvif/device_service", 
		m_platform->GetNetworkSetting()->GetAddress(),
		m_platform->GetOnvifPortNo()
	);
	DBG_PRINT("xaddr = %s", xaddr);

	char m_identification_name[255];
	char m_identification_location[255];
	m_platform->GetOnvifId(m_identification_name, m_identification_location);
	
	char name[255];
	sprintf(name, "onvif://www.onvif.org/name/%s", m_identification_name);
	char location[255];
	sprintf(location, "onvif://www.onvif.org/location/%s", m_identification_location);
	
	struct ONVIF_DEVICE_INFO device_info;
	m_platform->GetDeviceInfo(&device_info);
	
	char buff[1024];
	sprintf(
		buff,
		"onvif://www.onvif.org/type/Network_Transmitter "
		"onvif://www.onvif.org/type/video_encoder "
		"onvif://www.onvif.org/hardware/%s "
		"onvif://www.onvif.org/Profile/Streaming "
		"%s "
		"%s",
		device_info.Model,
		name,
		location
		);
	
	m_discovery_server = new MH_DiscoveryServer();
	m_discovery_server->Start(
		m_platform->GetNetworkSetting()->GetAddress(), 
		m_platform->GetEndpointReference(),
		"http://www.onvif.org/ver10/network/wsdl", 
		"NetworkVideoTransmitter", 
		buff,
		xaddr, 
		"1"
	);
	
	DBG_TRACE("Exit SZ_OnvifProcess::StartDiscovery");
	
	return true;
}

/**
 * Discoveryサービスを停止する
 */
void SZ_OnvifProcess::StopDiscovery()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StopDiscovery");
	
	if (m_discovery_server != NULL)
	{
		m_discovery_server->Stop();
		delete m_discovery_server;
		m_discovery_server = NULL;
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::StopDiscovery");
}

//-------------------------------------------------------------------------------

void SZ_OnvifProcess::GetOnvifId(char* name, char* location)
{
	m_platform->GetOnvifId(name, location);
}

bool SZ_OnvifProcess::SetOnvifId(const char* name, const char* location)
{
	StopDiscovery();

	if (!m_platform->SetOnvifId(name, location))
	{
		return false;
	}

	StartDiscovery();

	return true;
}

//-------------------------------------------------------------------------------

bool SZ_OnvifProcess::GetDiscoveryMode()
{
	return m_platform->GetDiscoveryEnable();
}

bool SZ_OnvifProcess::SetDiscoveryMode(bool discovery_enable)
{
	if (!m_platform->SetDiscoveryEnable(discovery_enable))
	{
		return false;
	}
	
	if (discovery_enable)
	{
		StartDiscovery();
	}
	else
	{
		StopDiscovery();
	}
	
	return true;
}

//-------------------------------------------------------------------------------

bool SZ_OnvifProcess::AddUserInfo(const ONVIF_USER_INFO* user_info)
{
	if (!m_platform->GetUserSetting()->AddUserInfo(user_info))
	{
		return false;
	}
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	int capture_num = camera->GetCaptureNum();
	for(int index = 0; index < capture_num; index++)
	{
		if (m_rtsp_server[index] != NULL)
		{
			m_rtsp_server[index]->AddUser(user_info->username, user_info->password);
		}
	}
	
	return true;
}

bool SZ_OnvifProcess::SetUserInfo(const ONVIF_USER_INFO* user_info)
{
	if (!m_platform->GetUserSetting()->SetUserInfo(user_info))
	{
		return false;
	}

	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	int capture_num = camera->GetCaptureNum();
	for(int index = 0; index < capture_num; index++)
	{
		if (m_rtsp_server[index] != NULL)
		{
			m_rtsp_server[index]->AddUser(user_info->username, user_info->password);
		}
	}
	
	return true;
}

bool SZ_OnvifProcess::DeleteUserInfo(const char* username)
{
	if (!m_platform->GetUserSetting()->DeleteUserInfo(username))
	{
		return false;
	}
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	int capture_num = camera->GetCaptureNum();
	for(int index = 0; index < capture_num; index++)
	{
		if (m_rtsp_server[index] != NULL)
		{
			m_rtsp_server[index]->RemoveUser(username);
		}
	}
	
	return true;
}

//-------------------------------------------------------------------------------

bool SZ_OnvifProcess::CreateProfile(const char* name, const char* token, ONVIF_PROFILE_INFO* profile_info)
{
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	MH_CaptureDevice* capture = camera->ReserveCaptureDevice();
	if (capture == NULL)
	{
		return false;
	}
	strcpy(profile_info->capture_device_id, capture->GetDeviceId());
	
	if (!m_platform->GetProfileSetting()->CreateProfile(name, token, profile_info))
	{
		camera->ReleaseCaptureDevice(capture);
		return false;
	}
	
	return true;
}

bool SZ_OnvifProcess::DeleteProfile(const char* token)
{
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	ONVIF_PROFILE_INFO profile_info;
	if (!m_platform->GetProfileSetting()->FindProfile(token, &profile_info))
	{
		return false;
	}
	
	int index = camera->GetCaptureIndex(profile_info.capture_device_id);
	if (index >= 0)
	{
		StopCameraDevice(index, &profile_info);
		
		MH_CaptureDevice* capture = camera->GetCaptureDevice(index);
		if (capture != NULL)
		{
			camera->ReleaseCaptureDevice(capture);
		}
	}
	
	if (!m_platform->GetProfileSetting()->DeleteProfile(profile_info.name))
	{
		return false;
	}
	
	return true;
}

//-------------------------------------------------------------------------------

bool SZ_OnvifProcess::AddVideoSource(const char* profile_token, const char* source_token)
{
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	ONVIF_PROFILE_INFO profile_info;
	if (!profile_setting->FindProfile(profile_token, &profile_info))
	{
		return false;
	}

	ONVIF_VIDEO_SOURCE_CONFIG_INFO video_source_config_info;
	if (!profile_setting->FindVideoSourceConfig(source_token, &video_source_config_info))
	{
		return false;
	}
	
	strcpy(profile_info.video_source_config_token, source_token);
	
	if (!profile_setting->SetProfile(&profile_info))
	{
		return false;
	}
	
	return RestartCameraDevice(&profile_info);
}

bool SZ_OnvifProcess::RemoveVideoSource(const char* profile_token)
{
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	ONVIF_PROFILE_INFO profile_info;
	if (!profile_setting->FindProfile(profile_token, &profile_info))
	{
		return false;
	}

	strcpy(profile_info.video_source_config_token, "");
	
	if (!profile_setting->SetProfile(&profile_info))
	{
		return false;
	}
	
	return RestartCameraDevice(&profile_info);
}

bool SZ_OnvifProcess::AddVideoEncoder(const char* profile_token, const char* encoder_token)
{
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	ONVIF_PROFILE_INFO profile_info;
	if (!profile_setting->FindProfile(profile_token, &profile_info))
	{
		return false;
	}

	ONVIF_VIDEO_ENCODER_CONFIG_INFO video_encoder_config_info;
	if (!profile_setting->FindVideoEncoderConfig(encoder_token, &video_encoder_config_info))
	{
		return false;
	}
	
	strcpy(profile_info.video_encoder_token, encoder_token);
	
	if (!profile_setting->SetProfile(&profile_info))
	{
		return false;
	}
	
	return RestartCameraDevice(&profile_info);
}

bool SZ_OnvifProcess::RemoveVideoEncoder(const char* profile_token)
{
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	ONVIF_PROFILE_INFO profile_info;
	if (!profile_setting->FindProfile(profile_token, &profile_info))
	{
		return false;
	}

	strcpy(profile_info.video_encoder_token, "");
	
	if (!profile_setting->SetProfile(&profile_info))
	{
		return false;
	}

	return RestartCameraDevice(&profile_info);
}

int SZ_OnvifProcess::AddPtz(const char* profile_token, const char* ptz_token)
{
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	ONVIF_PROFILE_INFO profile_info;
	if (!profile_setting->FindProfile(profile_token, &profile_info))
	{
		return false;
	}

	ONVIF_PTZ_CONFIG_INFO ptz_config_info;
	if (!profile_setting->FindPtzConfig(ptz_token, &ptz_config_info))
	{
		return false;
	}
	
	strcpy(profile_info.ptz_token, ptz_token);
	
	if (!profile_setting->SetProfile(&profile_info))
	{
		return false;
	}
	
	return true;
}

int SZ_OnvifProcess::RemovePtz(const char* profile_token)
{
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	ONVIF_PROFILE_INFO profile_info;
	if (!profile_setting->FindProfile(profile_token, &profile_info))
	{
		return false;
	}

	strcpy(profile_info.ptz_token, "");
	
	if (!profile_setting->SetProfile(&profile_info))
	{
		return false;
	}

	return true;
}

int SZ_OnvifProcess::UpdateHomePosition(const char* profile_token, float x, float y)
{
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	ONVIF_PROFILE_INFO profile_info;
	if (!profile_setting->FindProfile(profile_token, &profile_info))
	{
		return false;
	}

	profile_info.ptz_home_pos_x = x;
	profile_info.ptz_home_pos_y = y;
	
	if (!profile_setting->SetProfile(&profile_info))
	{
		return false;
	}

	return true;
}

bool SZ_OnvifProcess::ChangeVideoEncoderConfig(const char* token, int quality)
{
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	if (!profile_setting->ChangeVideoEncoderConfig(token, quality))
	{
		return false;
	}
	
	int count = profile_setting->GetProfileCount();
	DBG_PRINT("GetProfileCount = %d", count);
	
	for(int i = 0; i < count; i++)
	{
		struct ONVIF_PROFILE_INFO profile_info;
		profile_setting->GetProfile(i, &profile_info);
		
		if (strcmp(profile_info.video_source_config_token, token) == 0)
		{
			RestartCameraDevice(&profile_info);
		}
	}
	
	return true;
}

//-------------------------------------------------------------------------------

bool SZ_OnvifProcess::SetSnapShotPortNo(int port_no)
{
	DBG_TRACE("Enter SZ_OnvifProcess::SetSnapShotPortNo");
	DBG_PRINT("port_no = %d", port_no);
	
	if (!m_platform->SetSnapShotPortNo(port_no))
	{
		return false;
	}
	
	StopSnapShotServer();
	StartSnapShotServer();
	
	DBG_TRACE("Exit SZ_OnvifProcess::SetSnapShotPortNo");

	return true;
}

bool SZ_OnvifProcess::GetSnapShotUri(const char* profile_token, char* snap_shot_uri)
{
	DBG_TRACE("Enter SZ_OnvifProcess::GetSnapShotUri");
	DBG_PRINT("profile_token = %s", profile_token);
	
	struct ONVIF_PROFILE_INFO profile_info;
	if (!m_platform->GetProfileSetting()->FindProfile(profile_token, &profile_info))
	{
		DBG_PRINT("Error: FindProfile");
		return false;
	}
	
	sprintf(
		snap_shot_uri, 
		"http://%s:%d/%s.jpg", 
		m_platform->GetNetworkSetting()->GetAddress(),
		m_platform->GetSnapShotPortNo(),
		profile_token
	);
	
	DBG_TRACE("Exit SZ_OnvifProcess::GetSnapShotUri");
	
	return true;
}

bool SZ_OnvifProcess::SetRtspPortNo(int port_no)
{
	DBG_TRACE("Enter SZ_OnvifProcess::SetRtspPortNo");
	DBG_PRINT("port_no = %d", port_no);
	
	if (!m_platform->SetRtspPortNo(port_no))
	{
		return false;
	}
	
	//-------------------------------------
	// 全てのRTSPサーバーを再起動する
	//-------------------------------------
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	SZ_ProfileSetting* profile_setting = m_platform->GetProfileSetting();
	
	int count = profile_setting->GetProfileCount();
	for(int i = 0; i < count; i++)
	{
		struct ONVIF_PROFILE_INFO profile_info;
		profile_setting->GetProfile(i, &profile_info);
		
		DBG_PRINT("[%d] name  = %s", i, profile_info.name);
		DBG_PRINT("[%d] token = %s", i, profile_info.token);
		
		int index = camera->GetCaptureIndex(profile_info.capture_device_id);
		if (index <= 0)
		{
			StopStreaming(index, &profile_info);
			
			StartStreaming(index, camera->GetCaptureDevice(index), &profile_info);
		}
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::SetRtspPortNo");
	
	return true;
}

bool SZ_OnvifProcess::GetRtspUri(const char* profile_token, char* uri)
{
	DBG_TRACE("Enter SZ_OnvifProcess::GetRtspUri");
	DBG_PRINT("profile_token = %s", profile_token);
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	struct ONVIF_PROFILE_INFO profile_info;
	if (!m_platform->GetProfileSetting()->FindProfile(profile_token, &profile_info))
	{
		DBG_PRINT("Error: FindProfile");
		return false;
	}
	
	DBG_PRINT("capture_device_id = %s", profile_info.capture_device_id);
	int index = camera->GetCaptureIndex(profile_info.capture_device_id);
	if (index < 0)
	{
		DBG_PRINT("Error: GetCaptureIndex");
		return false;
	}
	
	DBG_PRINT("index = %d", index);
	MH_CaptureDevice* capture = camera->GetCaptureDevice(index);
	if (capture == NULL)
	{
		DBG_PRINT("Error: GetCaptureDevice");
		return false;
	}
	
	sprintf(
		uri, 
		"rtsp://%s:%d/%s", 
		m_platform->GetNetworkSetting()->GetAddress(), 
		m_platform->GetRtspPortNo(), 
		capture->GetDeviceId()
	);
	
	DBG_TRACE("Exit SZ_OnvifProcess::GetRtspUri");
	
	return true;
}

//-------------------------------------------------------------------------------

void SZ_OnvifProcess::SetRecordingMode(bool mode)
{
	m_platform->SetRecordingMode(mode);
}

bool SZ_OnvifProcess::GetRecordingMode()
{
	return 	m_platform->GetRecordingMode();
}

bool SZ_OnvifProcess::StartRecording()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StartRecording");
	
	if (m_recoder != NULL)
	{
		return false;
	}

	MH_CameraDevice* camera = m_platform->GetCameraDevice();

	MH_CaptureDevice* capture = camera->GetCaptureDevice(0);
	
	m_recoder = new MH_MediaRecoder();
	m_recoder->Start(
		capture->GetWidth(), 
		capture->GetHeight(), 
		m_platform->GetRecordingPath(),
		m_platform->GetRecordingInterval(),
		m_platform->GetRecordingFileNum(),
		m_platform->GetRecordingKeepSize()
	);

	capture->AddSink(m_recoder);
	
	DBG_TRACE("Exit SZ_OnvifProcess::StartRecording");
	
	return true;
}

bool SZ_OnvifProcess::StopRecording()
{
	DBG_TRACE("Enter SZ_OnvifProcess::StopRecording");

	if (m_recoder == NULL)
	{
		return true;
	}
	
	MH_CameraDevice* camera = m_platform->GetCameraDevice();
	
	MH_CaptureDevice* capture = camera->GetCaptureDevice(0);
	
	capture->RemoveSink(m_recoder);
	
	m_recoder->Stop();
	delete m_recoder;
	m_recoder = NULL;
	
	DBG_TRACE("Exit SZ_OnvifProcess::StopRecording");
	
	return true;
}

bool SZ_OnvifProcess::IsRecording()
{
	if (m_recoder == NULL)
	{
		return false;
	}
	return true;
}

//-------------------------------------------------------------------------------

bool SZ_OnvifProcess::IsSupportedPTZ()
{
	return m_platform->IsSupportedPTZ();
}

void SZ_OnvifProcess::AbsoluteMove(const char* profile_token, float x, float y, float sx, float sy)
{
	DBG_TRACE("Enter SZ_OnvifProcess::AbsoluteMove");
	DBG_PRINT("profile_token = %s", profile_token);
	DBG_PRINT("x = %f", x);
	DBG_PRINT("y = %f", y);
	
	ONVIF_PROFILE_INFO profile_info;
	if (!m_platform->GetProfileSetting()->FindProfile(profile_token, &profile_info))
	{
		return;
	}
	
	ONVIF_PTZ_CONFIG_INFO ptz_config_info;
	if (!m_platform->GetProfileSetting()->FindPtzConfig(profile_info.ptz_token, &ptz_config_info))
	{
		return;
	}

	SZ_PtzDevice* ptz = m_platform->GetPtzDevice(profile_info.ptz_token);
	if (ptz == NULL)
	{
		return;
	}
	
	if (sx == 0)
	{
		float cx = ptz->GetPosX();
		if (x > cx)
		{
			sx = ptz_config_info.default_speed_x;
		}
		else if (x < cx)
		{
			sx = ptz_config_info.default_speed_x * -1;
		}
	}
	if (sy == 0)
	{
		float cy = ptz->GetPosY();
		if (y > cy)
		{
			sy = ptz_config_info.default_speed_y;
		}
		else if (y < cy)
		{
			sy = ptz_config_info.default_speed_y * -1;
		}
	}

	ptz->AbsoluteMove(x, y, sx, sy);
	
	DBG_TRACE("Exit SZ_OnvifProcess::AbsoluteMove");
}

void SZ_OnvifProcess::RelativeMove(const char* profile_token, float x, float y, float sx, float sy)
{
	DBG_TRACE("Enter SZ_OnvifProcess::RelativeMove");
	DBG_PRINT("profile_token = %s", profile_token);
	DBG_PRINT("x = %f", x);
	DBG_PRINT("y = %f", y);
	
	ONVIF_PROFILE_INFO profile_info;
	if (!m_platform->GetProfileSetting()->FindProfile(profile_token, &profile_info))
	{
		return;
	}
	
	ONVIF_PTZ_CONFIG_INFO ptz_config_info;
	if (!m_platform->GetProfileSetting()->FindPtzConfig(profile_info.ptz_token, &ptz_config_info))
	{
		return;
	}
	
	SZ_PtzDevice* ptz = m_platform->GetPtzDevice(profile_info.ptz_token);
	if (ptz == NULL)
	{
		return;
	}
	
	if (sx == 0)
	{
		if (x > 0)
		{
			sx = ptz_config_info.default_speed_x;
		}
		else if (x < 0)
		{
			sx = ptz_config_info.default_speed_x * -1;
		}
	}
	if (sy == 0)
	{
		if (y > 0)
		{
			sy = ptz_config_info.default_speed_y;
		}
		else if (y < 0)
		{
			sy = ptz_config_info.default_speed_y * -1;
		}
	}
		
	ptz->RelativeMove(x, y, sx, sy);
	
	DBG_TRACE("Exit SZ_OnvifProcess::RelativeMove");
}

void SZ_OnvifProcess::ContinuousMove(const char* profile_token, float sx, float sy)
{
	SZ_PtzDevice* ptz = GetPtzDevice(profile_token);
	if (ptz == NULL)
	{
		return;
	}
	ptz->ContinuousMove(sx, sy);
}

void SZ_OnvifProcess::StopMove(const char* profile_token)
{
	SZ_PtzDevice* ptz = GetPtzDevice(profile_token);
	if (ptz == NULL)
	{
		return;
	}
	ptz->StopMove();
}

//-------------------------------------------------------------------------------

bool SZ_OnvifProcess::FactoryReset(int type)
{
	DBG_TRACE("Enter SZ_OnvifProcess::FactoryReset");
	
	StopService();
	
	m_platform->FactoryReset(type);
	
	StartService();
	
	DBG_TRACE("Exit SZ_OnvifProcess::FactoryReset");

	return true;
}

bool SZ_OnvifProcess::SystemReboot()
{
	DBG_TRACE("Enter SZ_OnvifProcess::SystemReboot");
	
	StopService();
	
	m_platform->SystemReboot();
	
	DBG_TRACE("Exit SZ_OnvifProcess::SystemReboot");
	
	return true;
}

//-------------------------------------------------------------------------------

bool SZ_OnvifProcess::OnMessage(int message_id, void* data, int size)
{
	DBG_TRACE("Enter SZ_OnvifProcess::OnMessage");
	
	switch(message_id)
	{
	case MSG_ID_NETWORK_CHANGE:
		DBG_PRINT("+++++++++++++++++++++++++++++");
		DBG_PRINT("+++ MSG_ID_NETWORK_CHANGE +++");
		DBG_PRINT("+++++++++++++++++++++++++++++");
		StopService();
		StartService();
		break;
	
	case MSG_ID_DETECT_MOTION:
		DBG_PRINT("++++++++++++++++++++++++++++");
		DBG_PRINT("+++ MSG_ID_DETECT_MOTION +++");
		DBG_PRINT("++++++++++++++++++++++++++++");
		SetMotionEvent(true);
		break;
	
	case MSG_ID_LOST_MOTION:
		DBG_PRINT("++++++++++++++++++++++++++");
		DBG_PRINT("+++ MSG_ID_LOST_MOTION +++");
		DBG_PRINT("++++++++++++++++++++++++++");
		SetMotionEvent(false);
		break;
	}
	
	DBG_TRACE("Exit SZ_OnvifProcess::OnMessage");

	return true;
}

//-------------------------------------------------------------------------------
