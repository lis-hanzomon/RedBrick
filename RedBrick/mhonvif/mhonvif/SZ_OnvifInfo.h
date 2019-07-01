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

#ifndef ONVIF_INFO_H
#define ONVIF_INFO_H

// デバイス情報
struct ONVIF_DEVICE_INFO
{
	char Manufacturer[40];			// 製造者名
	char Model[40];					// モデル名
	char FirmwareVersion[40];		// ファームウェアバージョン
	char SerialNumber[40];			// シリアルナンバー
	char HardwareId[40];			// ハードウェアID
};

// ユーザ情報
struct ONVIF_USER_INFO
{
	char username[80];
	char password[80];
	int level;
};

// プロファイル情報
struct ONVIF_PROFILE_INFO
{
	char name[80];
	char token[80];
	char video_source_config_token[80];
	char video_encoder_token[80];
	char ptz_token[80];
	float ptz_home_pos_x;
	float ptz_home_pos_y;
	int fixed;
	char capture_device_id[80];
};

// ビデオソース情報
struct ONVIF_VIDEO_SOURCE_INFO
{
	char token[80];
	int framerate;
	int width;
	int height;
};

// ビデオソース設定情報
struct ONVIF_VIDEO_SOURCE_CONFIG_INFO
{
	char name[80];
	char token[80];
	char source_token[80];
	int use_count;
	
	int x;
	int y;
	int width;
	int height;
};

#define VIDEO_ENCODING_H264			1
#define VIDEO_ENCODING_H264_HIGH	1
	
// ビデオエンコーダー設定情報
struct ONVIF_VIDEO_ENCODER_CONFIG_INFO
{
	char name[80];
	char token[80];
	int use_count;
	
	int width;
	int height;
	int bitrate;
	int quality;
};

// PTZ設定情報
struct ONVIF_PTZ_CONFIG_INFO
{
	char name[80];
	char token[80];
	char node_token[80];
	int use_count;
	
	float default_speed_x;
	float default_speed_y;
};

#endif
