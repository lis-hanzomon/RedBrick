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
#include "OnvifService.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"
#include "OnvifImpl.h"

/**
 * コンストラクタ
 */
OnvifService::OnvifService()
{
	DBG_TRACE("Enter OnvifService::OnvifService");
	
	DBG_TRACE("Exit OnvifService::OnvifService");
}

/**
 * デストラクタ
 */
OnvifService::~OnvifService()
{
	DBG_TRACE("Enter OnvifService::~OnvifService");
	
	DBG_TRACE("Exit OnvifService::~OnvifService");
}

/**
 * ONVIF I/F を起動する
 */
bool OnvifService::Start(unsigned short port_no)
{
	DBG_TRACE("Enter OnvifService::Start");
	DBG_PRINT("port_no       = %d", port_no);
	DBG_PRINT("snap_shot_url = %s", snap_shot_url);
	
	m_port_no = port_no;
	
	if (!MH_Thread::Start())
	{
		return false;
	}
	
	DBG_TRACE("Exit OnvifService::Start");

	return true;
}

/**
 * ONVIF I/F を実行する
 */
void OnvifService::OnMain()
{
	DBG_TRACE("Enter OnvifService::OnMain");
	
	RunOnvifService(m_port_no);
	
	DBG_TRACE("Exit OnvifService::OnMain");
}

/**
 * ONVIF I/F を終了する
 */
void OnvifService::OnReqTerminate()
{
	DBG_TRACE("Enter OnvifService::OnReqTerminate");
	
	StopOnvifService();
	
	DBG_TRACE("Exit OnvifService::OnReqTerminate");
}
