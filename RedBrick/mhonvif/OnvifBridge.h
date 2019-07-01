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

#ifndef ONVIF_BRIDGE_H
#define ONVIF_BRIDGE_H

#include "mhonvif/OnvifInfo.h"

#ifdef __cplusplus

#include "mhonvif/SZ_OnvifProcess.h"

bool OnvifBridgeInitialize(SZ_OnvifProcess* process);
void OnvifBridgeTerminate();

void SetMotionEvent(bool motion_detect);

extern "C" {
#endif

//-------------------------------------------------------------------------

void Bridge_GetDeviceInfo(struct ONVIF_DEVICE_INFO* device_info);

void Bridge_GetOnvifIdntification(char* name, char* location);
void Bridge_SetOnvifIdntification(const char* name, const char* location);

int Bridge_GetDiscoveryMode();
void Bridge_SetDiscoveryMode(int discovery_mode);

void Bridge_GetHostname(char* hostname);
void Bridge_SetHostname(const char* hostname);

//-------------------------------------------------------------------------

void Bridge_FactoryReset(int type);
void Bridge_SystemReboot();

//-------------------------------------------------------------------------

int Bridge_GetUserCount();
int Bridge_GetUserInfo(int index, struct ONVIF_USER_INFO* user_info);
int Bridge_FindUserInfo(const char* username, struct ONVIF_USER_INFO* user_info);
void Bridge_AddUserInfo(const struct ONVIF_USER_INFO* user_info);
void Bridge_DeleteUserInfo(const char* username);
void Bridge_SetUserInfo(const struct ONVIF_USER_INFO* user_info);

//-------------------------------------------------------------------------

void Bridge_GetNetwork(char* if_name, char* hw_address, int* use_dhcp, char* ip_address, int* prefix_len);
int Bridge_SetNetwork(const char* if_name, int use_dhcp, const char* ip_address, int prefix_len);

void Bridge_GetDnsAddress(int* use_dhcp, char* dns_address);
int Bridge_SetDnsAddress(int use_dhcp, const char* dns_address);

void Bridge_GetGatewayAddress(char* address);
int Bridge_SetGatewayAddress(const char* address);

int Bridge_GetOnvifPortNo();

int Bridge_GetSnapShotPortNo();
int Bridge_SetSnapShotPortNo(int port_no);

int Bridge_GetRtspPortNo();
int Bridge_SetRtspPortNo(int port_no);

const char* Bridge_GetIpAddres();

//-------------------------------------------------------------------------

int Bridge_GetVideoSourceCount();
int Bridge_GetVideoSource(int index, struct ONVIF_VIDEO_SOURCE_INFO* video_source_info);

int Bridge_GetVideoSourceConfigCount();
int Bridge_GetVideoSourceConfig(int index, struct ONVIF_VIDEO_SOURCE_CONFIG_INFO* video_source_config_info);
int Bridge_FindVideoSourceConfig(const char* token, struct ONVIF_VIDEO_SOURCE_CONFIG_INFO* video_source_config_info);

int Bridge_GetVideoEncoderConfigCount();
int Bridge_GetCompatibleVideoEncoderConfigCount(int width, int height);
int Bridge_GetVideoEncoderConfig(int index, struct ONVIF_VIDEO_ENCODER_CONFIG_INFO* video_encoder_config_info);
int Bridge_FindVideoEncoderConfig(const char* token, struct ONVIF_VIDEO_ENCODER_CONFIG_INFO* video_encoder_config_info);

int Bridge_GetSupportedResolutionCount();
void Bridge_GetSupportedResolution(int index, int* width, int* height);

int Bridge_ChangeVideoEncoderConfig(const char* encoder_token, int quality);

int Bridge_GetPtzConfigCount();
int Bridge_GetPtzConfig(int index, struct ONVIF_PTZ_CONFIG_INFO* ptz_config_info);
int Bridge_FindPtzConfig(const char* ptz_token, struct ONVIF_PTZ_CONFIG_INFO* ptz_config_info);

int Bridge_GetPtzStatus(const char* profile_token, float* x, float* y, int* is_moving);

//-------------------------------------------------------------------------

int Bridge_GetProfileCount();
int Bridge_GetProfile(int index, struct ONVIF_PROFILE_INFO* profile_info);
int Bridge_FindProfile(const char* token, struct ONVIF_PROFILE_INFO* profile_info);
int Bridge_CreateProfile(const char* name, const char* token, struct ONVIF_PROFILE_INFO* profile_info);
int Bridge_DeleteProfile(const char* token);

int Bridge_AddVideoSource(const char* profile_token, const char* source_token);
int Bridge_RemoveVideoSource(const char* profile_token);

int Bridge_AddVideoEncoder(const char* profile_token, const char* encoder_token);
int Bridge_RemoveVideoEncoder(const char* profile_token);

int Bridge_AddPtz(const char* profile_token, const char* ptz_token);
int Bridge_RemovePtz(const char* profile_token);

int Bridge_UpdateHomePosition(const char* profile_token, float x, float y);

int Bridge_GetSnapShotUri(const char* profile_token, char* uri);
int Bridge_GetRtspUri(const char* profile_token, char* uri);

//-------------------------------------------------------------------------

int Bridge_CreateMotionEvent(char* pull_point);
void Bridge_DeleteMotionEvent(const char* pull_point);
int Bridge_RenewMotionEvent(const char* pull_point);
int Bridge_WaitMotionEvent(const char* pull_point, int timeout, char* video_source_token, int* motion_detect);

//-------------------------------------------------------------------------

void Bridge_SetRecordingMode(int mode);
int Bridge_GetRecordingMode();

int Bridge_StartRecording();
int Bridge_StopRecording();
int Bridge_IsRecording();

//-------------------------------------------------------------------------

int Bridge_IsSupportedPTZ();
void Bridge_AbsoluteMove(const char* profile_token, float x, float y, float sx, float sy);
void Bridge_RelativeMove(const char* profile_token, float x, float y, float sx, float sy);
void Bridge_ContinuousMove(const char* profile_token, float sx, float sy);
void Brdige_StopMove(const char* profile_token);

//-------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
