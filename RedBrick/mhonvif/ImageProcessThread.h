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

#ifndef IMAGE_PROCESS_THREAD_H
#define IMAGE_PROCESS_THREAD_H

#include "mhengine/MH_Thread.h"
#include "mhengine/MH_Event.h"
#include "mhmedia/MH_CaptureDevice.h"
#include "MotionDetect.h"
#include "SnapShotWebServer.h"

#include <time.h>

class ImageProcessThread : public MH_Thread
{
private:
	MH_CaptureDevice* m_capture;
	MH_MediaPacket* m_frame;
	
	MotionDetect* m_motion_detect;
	SnapShotWebServer* m_snap_shot_server;
	
	MH_Event* m_event;
	
	char m_profile_token[80];
	MH_JpegWriter* m_jpeg_writer;
	
	unsigned char* m_jpeg_buff;
	int m_jpeg_buff_size;

	timespec m_snap_shot_time;
	
	int TimeSpecDiff(timespec* start, timespec* stop);
	
	bool UpdateSnapShot(MH_MediaPacket* frame, bool force);
	
protected:
	virtual void OnMain();
	virtual void OnReqTerminate();
	
public:
	ImageProcessThread(MH_CaptureDevice* capture, SnapShotWebServer* snap_shot_server);
	virtual ~ImageProcessThread();

	virtual bool Start(const char* profile_token, int src_width, int src_height, int out_width);
	virtual void Stop();
};

#endif
