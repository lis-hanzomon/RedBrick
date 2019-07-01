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
#include "mhmedia/MH_Media.h"
#include "mhmedia/ffmpeg_inc.h"
#include "mhengine/MH_CriticalSection.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

int lockmgr_cb_for_libav(void **mutex, enum AVLockOp op)
{
	if (op == AV_LOCK_CREATE)
	{
		*mutex = new MH_CriticalSection();
	}
	else
	{
		MH_CriticalSection* sync = (MH_CriticalSection*)*mutex;

		if (op == AV_LOCK_DESTROY)
		{
			delete sync;
		}
		else if (op == AV_LOCK_OBTAIN)
		{
			if (sync == NULL)
			{
				return -1;
			}
			sync->Lock();
		}
		else if (op == AV_LOCK_RELEASE)
		{
			if (sync == NULL)
			{
				return -1;
			}
			sync->Unlock();
		}
	}

	return 0;
}

static bool InitFFmpeg()
{
	unsigned version = avformat_version();
	DBG_PRINT("version = %d.%d.%d", (version >> 16) & 0xff, (version >> 8) & 0x0f, version & 0x0f);
	
	av_log_set_level(AV_LOG_DEBUG);

	av_register_all();

	av_lockmgr_register(lockmgr_cb_for_libav);

	return true;
}

static void TermFFmpeg()
{
	av_lockmgr_register(NULL);
}

bool MH_MediaInitialize()
{
	InitFFmpeg();
	
	return true;
}

void MH_MEDIA_H MH_MediaTerminate()
{
	TermFFmpeg();
}