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

#ifndef MH_RTSP_SERVER_THREAD_H
#define MH_RTSP_SERVER_THREAD_H

#include "MH_Media.h"

#include "mhengine/MH_Thread.h"
#include "MH_MediaPacketSrc.h"

#include <RTSPServer.hh>

/**
 * RTSP動画配信スレッド
 */
class MHMEDIA_API MH_RtspServerThread : public MH_Thread
{
private:
	MH_MediaPacketSrc* m_src;
	int m_max_store_sec;
	char m_session_name[80];
	short m_port;
	bool m_use_auth;
	char m_eventLoopWatchVariable;
	
	UserAuthenticationDatabase* m_authDB;

protected:
	virtual void OnMain();
	virtual void OnReqTerminate();

public:
	/**
	 * コンストラクタ
	 */
	MH_RtspServerThread();
	
	/**
	 * デストラクタ
	 */
	virtual ~MH_RtspServerThread();
	
	/**
	 * 初期化
	 */
	bool Create();
	
	/**
	 * 終了
	 */
	void Destroy();

	/**
	 * ユーザ追加
	 */
	void AddUser(const char* username, const char* password);
	
	/**
	 * ユーザ削除
	 */
	void RemoveUser(const char* username);
	
	/**
	 * 動画配信開始
	 */
	bool Start(MH_MediaPacketSrc* src, const char* session_name = "", short port = 8554, int m_max_store_sec = -1, bool use_auth = false);
};

#endif
