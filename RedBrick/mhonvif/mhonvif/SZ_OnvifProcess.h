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

#ifndef ONVIF_PROCESS_H
#define ONVIF_PROCESS_H

#include "SZ17.h"

#include "mhengine/MH_MessageTask.h"
#include "mhmedia/MH_RtspServerThread.h"
#include "mhmedia/MH_MediaRecoder.h"
#include "mhdiscovery/MH_DiscoveryServer.h"
#include "SZ_OnvifPlatform.h"

#include <vector>
#include <map>

class OnvifService;
class SnapShotWebServer;
class ImageProcessThread;

class SZ17_API SZ_OnvifProcess : public MH_MessageTask
{
private:
	SZ_OnvifPlatform* m_platform;
	MH_MediaRecoder* m_recoder;
	
	SnapShotWebServer* m_snapshot_server;
	std::vector<MH_RtspServerThread*> m_rtsp_server;
	std::vector<ImageProcessThread*> m_image_process;
	
	bool StartCameraDevice(int index, const ONVIF_PROFILE_INFO* profile_info);
	void StopCameraDevice(int index, const ONVIF_PROFILE_INFO* profile_info);
	bool RestartCameraDevice(const ONVIF_PROFILE_INFO* profile_info);

	bool StartSnapShotServer();
	void StopSnapShotServer();
	
	OnvifService* m_onvif_service;
	MH_DiscoveryServer* m_discovery_server;
	
	bool StartOnvifService();
	void StopOnvifService();

	bool StartDiscovery();
	void StopDiscovery();
	
	bool StartService();
	void StopService();

	bool StartStreaming(int index, MH_CaptureDevice* capture, const ONVIF_PROFILE_INFO* profile_info);
	void StopStreaming(int index, const ONVIF_PROFILE_INFO* profile_info);

protected:
	/**
	 * タスク初期化
	 */
	virtual bool OnTaskInitialize();
	
	/**
	 * タスク終了
	 */
	virtual void OnTaskTerminate();
	
	/**
	 * メッセージ受信
	 */
	virtual bool OnMessage(int message_id, void* data, int size);

public:
	/**
	 * コンストラクタ
	 */
	SZ_OnvifProcess(SZ_OnvifPlatform* setting);
	
	/**
	 * デストラクタ
	 */
	virtual ~SZ_OnvifProcess();
	
	SZ_OnvifPlatform* GetOnvifPlatform();
	MH_CameraDevice* GetCameraDevice();
	SZ_PtzDevice* GetPtzDevice(const char* profile_token);
	
	//----------------------------------------------------------------
	
	void GetOnvifId(char* name, char* location);
	bool SetOnvifId(const char* name, const char* location);
	
	bool GetDiscoveryMode();
	bool SetDiscoveryMode(bool discovery_enable);
	
	bool SetSnapShotPortNo(int port_no);
	bool SetRtspPortNo(int port_no);
	
	//----------------------------------------------------------------
	
	bool AddUserInfo(const ONVIF_USER_INFO* user_info);
	bool SetUserInfo(const ONVIF_USER_INFO* user_info);
	bool DeleteUserInfo(const char* username);
	
	//----------------------------------------------------------------
	
	bool CreateProfile(const char* name, const char* token, ONVIF_PROFILE_INFO* profile_info);
	bool DeleteProfile(const char* token);
	
	bool AddVideoSource(const char* profile_token, const char* source_token);
	bool RemoveVideoSource(const char* profile_token);
	
	bool AddVideoEncoder(const char* profile_token, const char* encoder_token);
	bool RemoveVideoEncoder(const char* profile_token);
	
	int AddPtz(const char* profile_token, const char* ptz_token);
	int RemovePtz(const char* profile_token);
	
	int UpdateHomePosition(const char* profile_token, float x, float y);

	bool ChangeVideoEncoderConfig(const char* token, int quality);
	
	bool GetSnapShotUri(const char* profile_token, char* uri);
	bool GetRtspUri(const char* profile_token, char* uri);
	
	//----------------------------------------------------------------
	
	void SetRecordingMode(bool mode);
	bool GetRecordingMode();
	
	bool StartRecording();
	bool StopRecording();
	bool IsRecording();
	
	//----------------------------------------------------------------

	bool IsSupportedPTZ();

	void AbsoluteMove(const char* profile_token, float x, float y, float sx, float sy);
	void RelativeMove(const char* profile_token, float x, float y, float sx, float sy);
	void ContinuousMove(const char* profile_token, float sx, float sy);
	void StopMove(const char* profile_token);

	//----------------------------------------------------------------

	bool FactoryReset(int type); 
	bool SystemReboot(); 
};

#endif
