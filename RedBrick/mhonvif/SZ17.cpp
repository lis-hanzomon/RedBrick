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
#include "mhonvif/SZ17.h"
#include "mhengine/MH_Engine.h"
#include "mhengine/MH_MessageQueueManager.h"
#include "mhmedia/MH_Media.h"
#include "mhdiscovery/MH_Discovery.h"
#include "MessageDef.h"

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

bool SZ17_Initialize(const char* filename)
{
	MH_Initialize(filename);
	MH_MediaInitialize();
	MH_DiscoveryInitialize();
	
	MH_MessageQueueManager::Initialize(1);
	MH_MessageQueueManager* queue_manager = MH_MessageQueueManager::GetInstance();
	queue_manager->CreateMessageQueue(QUEU_ID_ONVIF_PROCESS);
	
	return true;
}

void SZ17_Terminate()
{
	MH_MessageQueueManager::Terminate();

	MH_DiscoveryTerminate();
	MH_MediaTerminate();
	MH_Terminate();
}
