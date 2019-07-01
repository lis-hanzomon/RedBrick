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

#ifndef MH_JPEG_WRITER_H
#define MH_JPEG_WRITER_H

#include "MH_Media.h"

#include "ffmpeg_inc.h"

class MHMEDIA_API MH_JpegWriter
{
private:
	AVCodecContext* codec_ctx;
	SwsContext* sws_ctx;
	AVFrame* src_frame;
	AVFrame* sws_frame;

	AVFrame* AllocateVideoFrame(int width, int height);

public:
	MH_JpegWriter();
	virtual ~MH_JpegWriter();
	
	bool Open(int src_width, int src_height, int out_width, int out_height);
	void Close();
	
	int Write(const unsigned char* buff, int size, unsigned char* dst_buff, int dst_buff_size);
};

#endif
