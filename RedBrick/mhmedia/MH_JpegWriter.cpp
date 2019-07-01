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
#include "mhmedia/MH_JpegWriter.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

MH_JpegWriter::MH_JpegWriter()
	: codec_ctx(NULL)
	, sws_ctx(NULL)
	, src_frame(NULL)
	, sws_frame(NULL)
{
}

MH_JpegWriter::~MH_JpegWriter()
{
}

bool MH_JpegWriter::Open(int src_width, int src_height, int out_width, int out_height)
{
	DBG_TRACE("Enter MH_JpegWriter::Open");
	DBG_PRINT("src_width  = %d", src_width);
	DBG_PRINT("src_height = %d", src_height);
	DBG_PRINT("out_width  = %d", out_width);
	DBG_PRINT("out_height = %d", out_height);
	
	codec_ctx = avcodec_alloc_context3(NULL);
	if (codec_ctx == NULL)
	{
		DBG_PRINT("Error: avcodec_alloc_context3 = NULL");
		Close();
		return false;
	}

	codec_ctx->codec_type    = AVMEDIA_TYPE_VIDEO;
	codec_ctx->codec_id      = AV_CODEC_ID_MJPEG;
	codec_ctx->width         = out_width;
	codec_ctx->height        = out_height;
	codec_ctx->pix_fmt       = AV_PIX_FMT_YUVJ420P;
	codec_ctx->time_base.num = 1;
	codec_ctx->time_base.den = 1;

	AVCodec* codec = avcodec_find_encoder(codec_ctx->codec_id);
	if (codec == NULL)
	{
		DBG_PRINT("Error: avcodec_find_encoder = NULL");
		Close();
		return false;
	}

	DBG_PRINT("--- avcodec_open2 ---");
	int ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0)
	{
		DBG_PRINT("Error: avcodec_open2 = %d", ret);
		Close();
		return false;
	}
	
	src_frame = AllocateVideoFrame(src_width, src_height);
	if (src_frame == NULL)
	{
		DBG_PRINT("Error: AllocateFrame = NULL");
		Close();
		return false;
	}
	
	if (src_width != out_width || src_height != out_height)
	{
		DBG_PRINT("--- sws_getContext ---");

		sws_ctx = sws_getContext(
			src_width,
			src_height,
			AV_PIX_FMT_YUV420P,
			out_width,
			out_height,
			AV_PIX_FMT_YUV420P,
			SWS_FAST_BILINEAR,
			NULL,
			NULL,
			NULL
			);
		if (sws_ctx == NULL)
		{
			DBG_PRINT("Error: sws_getContext = NULL");
			Close();
			return false;
		}
		
		sws_frame = AllocateVideoFrame(out_width, out_height);
		if (sws_frame == NULL)
		{
			DBG_PRINT("Error: AllocateFrame = NULL");
			Close();
			return false;
		}
	}
	
	DBG_TRACE("Exit MH_JpegWriter::Open");
	
	return true;
}

void MH_JpegWriter::Close()
{
	DBG_TRACE("Enter MH_JpegWriter::Close");
	
	if (sws_ctx != NULL)
	{
		DBG_PRINT("sws_ctx");
		sws_freeContext(sws_ctx);
		sws_ctx = NULL;
	}
	
	if (sws_frame != NULL)
	{
		DBG_PRINT("sws_frame");
		av_frame_free(&sws_frame);
	}

	if (src_frame != NULL)
	{
		DBG_PRINT("src_frame");
		av_frame_free(&src_frame);
	}
	
	if (codec_ctx != NULL)
	{
		DBG_PRINT("avcodec_close - codec_ctx");
		avcodec_close(codec_ctx);
		DBG_PRINT("av_free - codec_ctx");
		av_free(codec_ctx);
		codec_ctx = NULL;
	}
	
	DBG_TRACE("Exit MH_JpegWriter::Close");
}

AVFrame* MH_JpegWriter::AllocateVideoFrame(int width, int height)
{
	DBG_TRACE("Enter MH_JpegWriter::AllocateVideoFrame");
	
	const int format = AV_PIX_FMT_YUV420P;
	
	AVFrame* frame = av_frame_alloc();
	if (frame == NULL)
	{
		DBG_PRINT("Error: avcodec_alloc_frame");
		return NULL;
	}

	int size = avpicture_get_size((AVPixelFormat)format, width, height);
	unsigned char* buff = (unsigned char*)av_malloc(size);
	if (buff == NULL)
	{
		DBG_PRINT("Error: av_malloc");
		av_frame_free(&frame);
		return NULL;
	}

	memset(buff,                  0x10, width * height    );
	memset(buff + width * height, 0x80, width * height / 2);

	avpicture_fill((AVPicture*)frame, buff, (AVPixelFormat)format, width, height);

	frame->format = format;
	frame->width  = width;
	frame->height = height;

	DBG_TRACE("Exit JpegWriter::AllocateVideoFrame");
	
	return frame;
}

int MH_JpegWriter::Write(const unsigned char* buff, int size, unsigned char* dst_buff, int dst_buff_size)
{
	DBG_TRACE("Enter MH_JpegWriter::Write");
	DBG_PRINT("size = %d", size);
	
	memcpy(src_frame->data[0], buff, size);
	
	AVFrame* wrk_frame = NULL;
	
	if (sws_ctx == NULL)
	{
		wrk_frame = src_frame;
	}
	else
	{
		int ret = sws_scale(
			sws_ctx,
			src_frame->data,
			src_frame->linesize,
			0,
			src_frame->height,
			sws_frame->data,
			sws_frame->linesize
			);
		if (ret < 0)
		{
			DBG_PRINT("Error: sws_scale = %d", ret);
			return false;
		}
		
		wrk_frame = sws_frame;
	}

	AVPacket jpeg_packet;
	av_init_packet(&jpeg_packet);
	jpeg_packet.data = NULL;
	jpeg_packet.size = 0;
	
	int got_frame = 0;
	int ret = avcodec_encode_video2(codec_ctx, &jpeg_packet, wrk_frame, &got_frame);
	if (ret < 0)
	{
		DBG_PRINT("Error: avcodec_encode_video2 = %d", ret);
		return false;
	}
	DBG_PRINT("got_frame = %d", got_frame);

	int jpeg_size = -1;
	
	if (got_frame)
	{
		DBG_PRINT("JPEG_PACKET_SIZE = %d", jpeg_packet.size);
		
		if (jpeg_packet.size <= dst_buff_size)
		{
			memcpy(dst_buff, jpeg_packet.data, jpeg_packet.size);
			jpeg_size = jpeg_packet.size;
		}
	}

	av_free_packet(&jpeg_packet);
	
	DBG_TRACE("Exit MH_JpegWriter::Write");
	
	return jpeg_size;
}
