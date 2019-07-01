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
#include "ImageProcessThread.h"
	
#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

ImageProcessThread::ImageProcessThread(MH_CaptureDevice* capture, SnapShotWebServer* snap_shot_server)
	: MH_Thread()
	, m_capture(capture)
	, m_frame(NULL)
	, m_motion_detect(NULL)
	, m_snap_shot_server(snap_shot_server)	
	, m_event(NULL)
	, m_jpeg_writer(NULL)
	, m_jpeg_buff(NULL)
	, m_jpeg_buff_size(0)
{
	DBG_TRACE("Enter ImageProcessThread::ImageProcessThread");
	
	DBG_TRACE("Exit ImageProcessThread::ImageProcessThread");
}

ImageProcessThread::~ImageProcessThread()
{
	DBG_TRACE("Enter ImageProcessThread::~ImageProcessThread");
	
	DBG_TRACE("Exit ImageProcessThread::~ImageProcessThread");
}

bool ImageProcessThread::Start(const char* profile_token, int src_width, int src_height, int out_width)
{
	DBG_TRACE("Enter ImageProcessThread::Start");
	DBG_PRINT("src_width  = %d", src_width);
	DBG_PRINT("src_height = %d", src_height);
	DBG_PRINT("out_width  = %d", out_width);
	
	strcpy(m_profile_token, profile_token);
	
	int out_height = out_width * src_height /src_width;
	DBG_PRINT("out_height = %d", out_height);
	
	m_jpeg_writer = new MH_JpegWriter();
	if (!m_jpeg_writer->Open(src_width, src_height, out_width, out_height))
	{
		DBG_PRINT("Error: MH_JpegWriter::Open");
		return false;
	}

	m_jpeg_buff_size = src_width * src_height * 2;
	m_jpeg_buff = new unsigned char[m_jpeg_buff_size];

	clock_gettime(CLOCK_MONOTONIC, &m_snap_shot_time);
	
	m_event = new MH_Event();
	m_event->Create();
	
	m_frame = new MH_MediaPacket(NULL);
	m_frame->Allocate(src_width * src_height + src_width * src_height / 2);
	
  	// 静止画を取得する
	m_capture->SnapShot(m_frame);
	
	// 動体検出を行う
	m_motion_detect = new MotionDetect();
	m_motion_detect->Create(src_width, src_height);
	m_motion_detect->FirstFrame(m_frame);
	
	// スナップショットを更新する
	UpdateSnapShot(m_frame, true);
	
	DBG_TRACE("Exit ImageProcessThread::Start");
	
	return MH_Thread::Start();
}

void ImageProcessThread::Stop()
{
	DBG_TRACE("Enter ImageProcessThread::Stop");
	
	MH_Thread::Stop();
	
	if (m_motion_detect != NULL)
	{
		m_motion_detect->Close();
		delete m_motion_detect;
		m_motion_detect = NULL;
	}
	
	if (m_frame != NULL)
	{
		m_frame->Free();
		delete m_frame;
		m_frame = NULL;
	}
	
	if (m_jpeg_writer != NULL)
	{
		delete m_jpeg_writer;
		m_jpeg_writer = NULL;
	}
	
	if (m_jpeg_buff != NULL)
	{
		delete [] m_jpeg_buff;
		m_jpeg_buff = NULL;
		m_jpeg_buff_size = 0;
	}
	
	DBG_TRACE("Exit ImageProcessThread::Stop");
}
	
void ImageProcessThread::OnMain()
{
	DBG_TRACE("Enter ImageProcessThread::OnMain");
	
	while(!IsTerminate())
	{
		// 50ms間隔で処理を実行する
		m_event->Wait(50);
		
		if (IsTerminate())
		{
			break;
		}
		
	  	// 静止画を取得する
		m_capture->SnapShot(m_frame);

		// 動体検出を行う
		m_motion_detect->NextFrame(m_frame);

		// スナップショットを更新する
		UpdateSnapShot(m_frame, false);
	}
	
	DBG_TRACE("Exit ImageProcessThread::OnMain");
}

void ImageProcessThread::OnReqTerminate()
{
	DBG_TRACE("Enter ImageProcessThread::OnReqTerminate");
	
	m_event->Set();
	
	DBG_TRACE("Exit ImageProcessThread::OnReqTerminate");
}

bool ImageProcessThread::UpdateSnapShot(MH_MediaPacket* frame, bool force)
{
	// 静止画取得時間を記憶する
	timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);
	
	// 前回作成からの時間経過を確認する
	if (!force)
	{
		// 3秒毎でスナップショットを更新する
		int sec = TimeSpecDiff(&m_snap_shot_time, &current_time);
		if (sec < 3)
		{
			// まだ作成しない
			return true;
		}
	}
	
	DBG_TRACE("Enter ImageProcessThread::UpdateSnapShot");
	
	// JPEGファイル形式で保存する
	int jpeg_size = m_jpeg_writer->Write(frame->m_data, frame->m_size, m_jpeg_buff, m_jpeg_buff_size);
	
	// JPEGファイル更新を通知する
	m_snap_shot_server->UpdateSnapShot(m_profile_token, m_jpeg_buff, jpeg_size);

	// 静止画取得時間を記憶する
	m_snap_shot_time = current_time;
	
	DBG_TRACE("Exit ImageProcessThread::UpdateSnapShot");
	
	return true;
}


int ImageProcessThread::TimeSpecDiff(timespec* start, timespec* stop)
{
	int sec = 0;
	
    if ((stop->tv_nsec - start->tv_nsec) < 0) 
    {
        sec = stop->tv_sec - start->tv_sec - 1;
    } 
    else 
    {
        sec = stop->tv_sec - start->tv_sec;
    }
    
    return sec;
}
