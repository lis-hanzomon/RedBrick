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

#ifndef MH_MEDIA_RECODER_H
#define MH_MEDIA_RECODER_H

#include "MH_Media.h"

#include "mhengine/MH_Event.h"
#include "mhengine/MH_Thread.h"
#include "MH_MediaPacketSink.h"
#include "MH_MediaPacketStream.h"

class MHMEDIA_API MH_MediaRecoder : public MH_Thread,
							        public MH_MediaPacketSink
{
private:
	int m_width;
	int m_height;
	char m_path[255];
	double m_div_sec;
	int m_store_num;
	int m_keep_size;
	
	MH_MediaPacketStream* m_stream;

	FILE* m_fp;							// TSファイル出力

	/**
	 * エラー発生フラグ
	 */
	bool m_error;

	void KeepFreeArea();

	/**
	 * TSを出力する
	 */
	static int stubWritePacket(void* opaque, unsigned char* buf, int buf_size);
	int WritePacket(unsigned char* buf, int buf_size);

protected:
	virtual void OnMain();
	virtual void Push(MH_MediaPacket* packet);
	virtual void OnReqTerminate();

public:
	MH_MediaRecoder();
	virtual ~MH_MediaRecoder();
	
	bool Start(int width, int height, const char* path, double div_sec, int store_num, int keep_size);
	void Stop();
};

#endif
