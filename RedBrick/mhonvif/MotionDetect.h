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

#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H

#include "mhmedia/MH_MediaPacket.h"

#include <opencv2/opencv.hpp>

class MotionDetect
{
private:
	int m_width;
	int m_height;
	
	cv::Mat* m_first_frame;
	cv::Mat* m_next_frame;

	int m_motion_detect_cnt;
	int m_motion_lost_cnt;
	bool m_motion_detect;

public:
	MotionDetect();
	virtual ~MotionDetect();
	
	bool Create(int width, int height);
	void Close();
	
	void FirstFrame(MH_MediaPacket* frame);
	void NextFrame(MH_MediaPacket* frame);
};

#endif
