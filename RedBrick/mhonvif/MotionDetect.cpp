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
#include "MotionDetect.h"
#include "mhengine/MH_MessageQueueManager.h"
#include "MessageDef.h"

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MotionDetect::MotionDetect()
	: m_first_frame(NULL)
	, m_next_frame(NULL)
	, m_motion_detect_cnt(0)
	, m_motion_lost_cnt(0)
	, m_motion_detect(false)
{
}

MotionDetect::~MotionDetect()
{
}

bool MotionDetect::Create(int width, int height)
{
	DBG_TRACE("Enter MotionDetect::Create");
	
	m_width  = width;
	m_height = height;

	m_first_frame = new cv::Mat(m_height, m_width, CV_8UC1);
	m_next_frame  = new cv::Mat(m_height, m_width, CV_8UC1);

	m_motion_detect = false;
	
	DBG_TRACE("Exit MotionDetect::Create");
	
	return true;
}

void MotionDetect::Close()
{
	if (m_first_frame != NULL)
	{
		delete m_first_frame;
		m_first_frame = NULL;
	}
	if (m_next_frame != NULL)
	{
		delete m_next_frame;
		m_next_frame = NULL;
	}
}

void MotionDetect::FirstFrame(MH_MediaPacket* frame)
{
	DBG_TRACE("Enter MotionDetect::FirstFrame");

	memcpy(m_first_frame->data, frame->m_data, m_width * m_height);
	
	DBG_TRACE("Exit MotionDetect::FirstFrame");
}

void MotionDetect::NextFrame(MH_MediaPacket* frame)
{
	// DBG_TRACE("Enter MotionDetect::NextFrame");

	memcpy(m_next_frame->data, frame->m_data, m_width * m_height);
	
	// 2枚の画像の差分を算出する
	cv::Mat diff;
	cv::absdiff(*m_first_frame, *m_next_frame, diff);

	// ヒストグラムを算出
	int image_num = 1;
	int channels[] = { 0 };
	cv::MatND hist;
	int dim_num = 1;
	int bin_num = 3;
	int bin_nums[] = { bin_num };
	float range[] = { 0, 256 };
	const float *ranges[] = { range };
	cv::calcHist(&diff, image_num, channels, cv::Mat(), hist, dim_num, bin_nums, ranges);
    		
 	MH_MessageQueue* queue = MH_MessageQueueManager::GetInstance()->GetMessageQueue(QUEU_ID_ONVIF_PROCESS);
 	
 	// 値の変化が大きい場合は、動体検出とみなす
	// DBG_PRINT("Motion [%d] : %f %f %f", m_motion_detect_cnt, hist.at<float>(0), hist.at<float>(1), hist.at<float>(2));
    if (hist.at<float>(1) > 5)
    {
    	if (!m_motion_detect)
    	{
			m_motion_detect_cnt++;
			
			DBG_PRINT("Motion Detect [%d] : %f %f %f", m_motion_detect_cnt, hist.at<float>(0), hist.at<float>(1), hist.at<float>(2));
	
			if (2 <= m_motion_detect_cnt)
			{
				m_motion_detect_cnt = 0;
	   			m_motion_detect     = true;
	   			
				// 動体検出
	   			queue->SendMessage(MSG_ID_DETECT_MOTION, NULL, 0);
	   		}
		}
		m_motion_lost_cnt = 0;
	}
	else
	{
    	if (m_motion_detect)
    	{
			m_motion_lost_cnt++;
			
			DBG_PRINT("Motion Lost [%d]  : %f %f %f", m_motion_lost_cnt, hist.at<float>(0), hist.at<float>(1), hist.at<float>(2));
			
			if (6 <= m_motion_lost_cnt)
			{
				m_motion_lost_cnt = 0;
	   			m_motion_detect = false;

				// 動体未検出
	   			queue->SendMessage(MSG_ID_LOST_MOTION, NULL, 0);
	   		}
		}
		m_motion_detect_cnt = 0;
	}

	cv::Mat* temp = m_first_frame;
	m_first_frame = m_next_frame;
	m_next_frame  = temp;
    
	// DBG_TRACE("Exit MotionDetect::NextFrame");
}