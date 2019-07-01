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
#include <unistd.h>
#include "mhmedia/MH_MediaRecoder.h"
#include "mhmedia/ffmpeg_inc.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

//---------------------------------------------------------------------------------------------------

/**
 * �R���X�g���N�^
 */
MH_MediaRecoder::MH_MediaRecoder()
	: m_stream(NULL)
{
	DBG_TRACE("Enter MH_MediaRecoder::MH_MediaRecoder");
	
	DBG_TRACE("Exit MH_MediaRecoder::MH_MediaRecoder");
}

/**
 * �f�X�g���N�^
 */
MH_MediaRecoder::~MH_MediaRecoder()
{
	DBG_TRACE("Enter MH_MediaRecoder::~MH_MediaRecoder");
	
	DBG_TRACE("Exit MH_MediaRecoder::~MH_MediaRecoder");
}

//---------------------------------------------------------------------------------------------------

/**
 * �^��J�n
 */
bool MH_MediaRecoder::Start(int width, int height, const char* path, double div_sec, int store_num, int keep_size)
{
	DBG_TRACE("Enter MH_MediaRecoder::Start");
	DBG_PRINT("width      = %d", width);
	DBG_PRINT("height     = %d", height);
	DBG_PRINT("path       = %s", path);
	DBG_PRINT("div_sec    = %f", div_sec);
	DBG_PRINT("store_num  = %d", store_num);
	DBG_PRINT("keep_size  = %d", keep_size);
	
	m_width  = width;
	m_height = height;
	
	strcpy(m_path, path);
	m_div_sec = div_sec;

	m_store_num = store_num;
	m_keep_size = keep_size;
	
	m_error = false;
	
	m_stream = new MH_MediaPacketStream("Recoder");
	if (!m_stream->Open())
	{
		Stop();
		return false;
	}

	if (!MH_Thread::Start())
	{
		Stop();
		return false;
	}

	DBG_TRACE("Enter MH_MediaRecoder::Exit");
	
	return true;
}

/**
 * �^��I��
 */
void MH_MediaRecoder::Stop()
{
	DBG_TRACE("Enter MH_MediaRecoder::Stop");
	
	MH_Thread::Stop();
	
	if (m_stream != NULL)
	{
		m_stream->Close();
		delete m_stream;
		m_stream = NULL;
	}
	
	DBG_TRACE("Exit MH_MediaRecoder::Stop");
}

//---------------------------------------------------------------------------------------------------

/**
 * �^�惁�C��
 */
void MH_MediaRecoder::OnMain()
{
	DBG_TRACE("Enter MH_MediaRecoder::OnMain");
	
	//---------------------------------
	// �r�f�I�̃p�[�T�[�𐶐�����
	//---------------------------------
	
	int ret = 0;
	
	AVCodecParserContext* video_parser_ctx = av_parser_init(AV_CODEC_ID_H264);
	if(video_parser_ctx == NULL)
	{
		DBG_PRINT("Error: av_parser_init(AV_CODEC_ID_H264)");
		return;
	}
	
	AVCodecContext* video_codec_ctx = avcodec_alloc_context3(NULL);
	if(video_codec_ctx == NULL)
	{
		DBG_PRINT("Error: avcodec_alloc_context3");
		return;
	}
	
	video_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	video_codec_ctx->codec_id   = AV_CODEC_ID_H264;
	
	AVCodec* video_codec = avcodec_find_decoder(video_codec_ctx->codec_id);
	if(video_codec == NULL)
	{
		DBG_PRINT("Error: avcodec_find_decoder");
		return;
	}

	ret = avcodec_open2(video_codec_ctx, video_codec, NULL);
	if(ret < 0)
	{
		DBG_PRINT("Error: avcodec_open2 = %d", ret);
		return;
	}

	// -----------------------------------------------------------------------------------
	// �t�@�C���o�͂��s��
	// -----------------------------------------------------------------------------------

	AVFormatContext* ic = NULL;
	
	long long first_video_dts = -1;
	long long last_div_dts = 0;
	double next_divpoint = 0;
	long long last_video_dts = -1;
	
	AVRational ts_time_base;
	ts_time_base.num = 1;
	ts_time_base.den = 90000;

	// H.264�p�P�b�g�����o�b�t�@(128KB)
	unsigned char* packet_data = (unsigned char*)malloc(1024 * 128);
	int packet_size = 0;
	long long packet_pts = -1;
	long long packet_dts = -1;
	
	// SPS/PPS���o�p�o�b�t�@
	unsigned char video_extradata[256];
	memset(video_extradata, 0x00, 256);
	int video_extradata_size = 0;
	
	// TS�����o�b�t�@(256KB)
	// ��TS�p�P�b�g���o�͂���o�b�t�@�Ȃ̂ŁA188�Ŋ���؂��T�C�Y�ɂ��Ă���B
	int stream_buffer_size = (256 * 1024) / 188 * 188;
	unsigned char* stream_buffer = (unsigned char*)malloc(stream_buffer_size);
	
	DBG_PRINT("### DO RECORDING LOOP ###");
	
	while(!IsTerminate())
	{
		MH_MediaPacket* packet = m_stream->Pop();
		if (packet == NULL)
		{
			DBG_PRINT("### Recording Cancel Detect ###");
			break;
		}

		// �X�^�[�g�R�[�h��t������
		const char start_code[3] = { 0x00, 0x00, 0x01 };
		unsigned char next_packet_data[1024 * 128];
		memcpy(next_packet_data, start_code, 3);
		memcpy(next_packet_data + 3, packet->m_data, packet->m_size);
		int next_packet_size = 3 + packet->m_size;
		
		// ���Ԃ̒P�ʂ����킹�� (��sec -> 1/90000sec)
		long long next_packet_pts = (long long)((double)packet->m_pts / 1000000 * ts_time_base.den);
		long long next_packet_dts = (long long)((double)packet->m_dts / 1000000 * ts_time_base.den);

		if (packet_pts == next_packet_pts)
		{
			// �������Ԃ̃p�P�b�g��1�ɂ܂Ƃ߂�
			// ��SPS,PPS,�f�[�^���P�ɂ���B
			memcpy(packet_data + packet_size, next_packet_data, next_packet_size);
			packet_size += next_packet_size;
		}
		else
		{
			if (packet_size > 0)
			{
				// �~�ς����p�P�b�g����������
				int i = video_parser_ctx->parser->split(video_codec_ctx, packet_data, packet_size); 
				if (i > 0 && video_extradata_size == 0)
				{
					//--------------------------------
					// ���߂�SPS�EPPS�����o����
					//--------------------------------
					
					DBG_PRINT("### find sps, pps = %d", i);
					
					video_extradata_size = i;
					memcpy(video_extradata, packet_data, i);
				}
				if(video_extradata_size > 0)
				{
					//------------------------------------------------------
					// SPS�EPPS���o�ς݂ł���΁A����f�[�^�̕ۑ����s��
					//------------------------------------------------------
					
					int vp_pos = 0;

					while (vp_pos < packet_size)
					{
						unsigned char* m_outbuf = NULL;
						int m_outbuf_size = 0;

						int ret = av_parser_parse2(
							video_parser_ctx,
							video_codec_ctx,
							&m_outbuf,
							&m_outbuf_size,
							packet_data + vp_pos,
							packet_size - vp_pos,
							packet_pts,
							packet_dts,
							0
							);
						if (ret < 0)
						{
							break;
						}

						vp_pos += ret;

						if ((m_outbuf != NULL) && (m_outbuf_size > 0))
						{
							//------------------------------------------------------------------------------
							// �r�f�I��PTS���X�V����
							//------------------------------------------------------------------------------

							long long video_pts = video_parser_ctx->pts;
							long long video_dts = video_parser_ctx->dts;

							// �擪�� PTS & DTS ���L������
							if (first_video_dts == -1 && video_dts != AV_NOPTS_VALUE)
							{
								// �S�̂̃X�^�[�g���Ԃ�ۑ�
								first_video_dts = video_dts;
								last_video_dts = video_dts;
								DBG_PRINT("first_video_dts = %lld", first_video_dts);
							}

							//------------------------------------------------------------------------------
							// �t�@�C���o�͒��ŃL�[�t���[�����擾�����ꍇ�́AHLS�̃t�@�C�������̔�����s��
							// ���L�[�t���[���ŕ������Ȃ��ƁA�Đ����Ɋ��炩�ɍĐ�����Ȃ��B
							//------------------------------------------------------------------------------

							if (ic != NULL && video_parser_ctx->key_frame == 1)
							{
								// �o�ߎ��Ԃ��Z�o
								long long check_video_dts = 0;
								if (first_video_dts <= last_video_dts)
								{
									check_video_dts = av_rescale_q(last_video_dts - first_video_dts, ts_time_base, ic->streams[0]->time_base);
								}
								else
								{
									check_video_dts  = av_rescale_q(0x200000000LL - first_video_dts + last_video_dts, ts_time_base, ic->streams[0]->time_base);
								}
								double duration = (double)check_video_dts / (double)ts_time_base.den;
								if (next_divpoint < duration)
								{
									//---------------------------
									// �t�@�C������ւ�
									//---------------------------

									DBG_PRINT("++++++++++++++++++++++++++++");
									DBG_PRINT("last_video_dts = %lld", last_video_dts);
									DBG_PRINT("video end      = %f", duration);
									DBG_PRINT("++++++++++++++++++++++++++++");

									// �t�@�C�������
									av_write_trailer(ic);

									// �X�g���[���o�͂��J������
									av_free(ic->pb);
									ic->pb = NULL;
									avformat_free_context(ic);
									ic = NULL;
									
									// �^��t�@�C�������
									fclose(m_fp);
									m_fp = NULL;
								}
							}

							if (ic == NULL && first_video_dts >= 0)
							{
								DBG_PRINT("+++++++++++++++++++++++++");
								DBG_PRINT("+++ Write New TS File +++");

								//--------------------------------------
								// �󂫗e�ʂ��m�ۂ���
								//--------------------------------------

								KeepFreeArea();

								//--------------------------------------
								// �^��t�@�C���𐶐�����
								//--------------------------------------

								SYSTEMTIME current_time;
								GetLocalTime(&current_time);

								char filename[255];
								sprintf(
									filename, 
									"%s/%04d%02d%02d%02d%02d%02d.ts", 
									m_path, 
									current_time.wYear,
									current_time.wMonth,
									current_time.wDay,
									current_time.wHour,
									current_time.wMinute,
									current_time.wSecond
								);
								DBG_PRINT("filename = %s", filename);
								m_fp = fopen(filename, "w");
								if(m_fp == NULL)
								{
									DBG_PRINT("Error: fopen(%s)", filename);
									m_error = true;
									break;
								}

								//--------------------------------------------
								// �t�@�C���o�͂��쐬
								//--------------------------------------------

								ic = avformat_alloc_context();
								if (ic == NULL)
								{
									DBG_PRINT("Error: avformat_alloc_context");
									m_error = true;
									break;
								}

								// �t�H�[�}�b�g��ݒ�
								AVOutputFormat* fmt = av_guess_format(NULL, "dummy.ts", NULL);
								ic->oformat = fmt;

								// �摜��ǉ�
								AVStream* st = avformat_new_stream(ic, NULL);
								st->codec->codec_type     = AVMEDIA_TYPE_VIDEO;
								st->codec->codec_id       = AV_CODEC_ID_H264;
								st->codec->width          = m_width;
								st->codec->height         = m_height;
								st->time_base.num         = ts_time_base.num;
								st->time_base.den         = ts_time_base.den;
								st->codec->extradata_size = video_extradata_size;
								
								st->codec->extradata = (uint8_t*)av_malloc(video_extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
								memcpy(st->codec->extradata, video_extradata, video_extradata_size);
								memset(st->codec->extradata + video_extradata_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

								if (fmt->flags & AVFMT_GLOBALHEADER)
								{
									st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
								}

								// �������o�͂�ݒ�
								ic->pb = avio_alloc_context(
									stream_buffer,
									stream_buffer_size,
									1,
									this,
									NULL,
									stubWritePacket,
									NULL
									);
								if (ic->pb == NULL)
								{
									DBG_PRINT("Error: avio_alloc_context");
									m_error = true;
									break;
								}

								// �w�b�_���o��
								ret = avformat_write_header(ic, NULL);
								if (ret < 0)
								{
									DBG_PRINT("Error: avformat_write_header");
									m_error = true;
									break;
								}

								// �����|�C���g���L������
								last_div_dts = last_video_dts;
								next_divpoint += m_div_sec;

								DBG_PRINT("last_div_dts = %lld", last_div_dts);

								DBG_PRINT("+++ Start OK +++");
								DBG_PRINT("++++++++++++++++");
							}

							if(ic != NULL)
							{
								AVPacket inpkt;
								av_init_packet(&inpkt);

								inpkt.stream_index = 0;
								inpkt.data = m_outbuf;
								inpkt.size = m_outbuf_size;

								//---------------------
								// PTS���Z�o����
								//---------------------

								if (first_video_dts <= video_pts)
								{
									inpkt.pts = av_rescale_q(video_pts - first_video_dts, ts_time_base, ic->streams[0]->time_base);
								}
								else
								{
									inpkt.pts = av_rescale_q(0x200000000LL - first_video_dts + video_pts, ts_time_base, ic->streams[0]->time_base);
								}

								//---------------------
								// DTS���Z�o����
								//---------------------

								if (first_video_dts <= video_dts)
								{
									inpkt.dts = av_rescale_q(video_dts - first_video_dts, ts_time_base, ic->streams[0]->time_base);
								}
								else
								{
									inpkt.dts = av_rescale_q(0x200000000LL - first_video_dts + video_dts, ts_time_base, ic->streams[0]->time_base);
								}

								last_video_dts = video_dts;

								//-----------------------
								// TS�ɏo�͂���
								//-----------------------

								ret = av_interleaved_write_frame(ic, &inpkt);
								if (ret < 0)
								{
									DBG_PRINT("Error: av_interleaved_write_frame(video) = %d", ret);
									av_free_packet(&inpkt);
									m_error = true;
									break;
								}

								av_free_packet(&inpkt);
							}
						}
					}
					
					if(m_error)
					{
						break;
					}
				}
			}
			
			// ����̃p�P�b�g���L������
			memcpy(packet_data, next_packet_data, next_packet_size);
			packet_size = next_packet_size;
			packet_pts = next_packet_pts;
			packet_dts = next_packet_dts;
		}
		
		packet->ReleaseRef();
	}
	
	if(video_parser_ctx != NULL)
	{
		av_parser_close(video_parser_ctx);
		video_parser_ctx = NULL;
	}
	if(video_codec_ctx != NULL)
	{
		avcodec_close(video_codec_ctx);
		av_free(video_codec_ctx);
		video_codec_ctx = NULL;
	}
	
	if(ic != NULL)
	{
		// �t�@�C�������
		av_write_trailer(ic);
		
		// �X�g���[���o�͂��J������
		av_free(ic->pb);
		ic->pb = NULL;
		avformat_free_context(ic);
		ic = NULL;
	}
	
	if (m_fp != NULL)
	{
		// �^��t�@�C�������
		fclose(m_fp);
		m_fp = NULL;
	}
	
	if (packet_data != NULL)
	{
		free(packet_data);
	}
	if (stream_buffer != NULL)
	{
		free(stream_buffer);
	}
	
	DBG_TRACE("Exit MH_MediaRecoder::OnMain");
}

/**
 * �f�[�^���o�͂���
 */
int MH_MediaRecoder::stubWritePacket(void *opaque, unsigned char *buf, int buf_size)
{
	MH_MediaRecoder* own = (MH_MediaRecoder*)opaque;
	return own->WritePacket(buf, buf_size);
}
int MH_MediaRecoder::WritePacket(unsigned char *buf, int buf_size)
{
	// DBG_TRACE("Enter MH_MediaRecoder::WritePacket");
	// DBG_PRINT("buf_size = %d", buf_size);
	
	if(m_fp != NULL)
	{
		int ret = 0;
		
		ret = fwrite(buf, 1, buf_size, m_fp);
		if(ret < buf_size)
		{
			// �t�@�C���ۑ��G���[
			DBG_PRINT("Error: fwrite");
			m_error = true;
			return -1;
		}
	}
	
	// DBG_TRACE("Exit MH_MediaRecoder::WritePacket");
	
	return buf_size;
}

//---------------------------------------------------------------------------------------------------

void MH_MediaRecoder::Push(MH_MediaPacket* packet)
{
	m_stream->Push(packet);
}

//---------------------------------------------------------------------------------------------------

void MH_MediaRecoder::OnReqTerminate()
{
	DBG_TRACE("Enter MH_MediaRecoder::OnReqTerminate");
	
	m_stream->Cancel();
	
	DBG_TRACE("Exit MH_MediaRecoder::OnReqTerminate");
}

//---------------------------------------------------------------------------------------------------

void MH_MediaRecoder::KeepFreeArea()
{
	std::list<std::string> filenames;
	
	if (-1 < m_store_num || -1 < m_keep_size)
	{
		// �t�@�C���ꗗ���擾����
		GetFiles(m_path, &filenames);
		DBG_PRINT("filenames.size = %d", filenames.size());
	}

	if (-1 < m_store_num)
	{
		// �t�@�C����������ȏ�쐬���Ȃ�
		while(m_store_num < filenames.size())
		{
			std::string filename = *(filenames.begin());
			DBG_PRINT("erase by num: %s", filename.c_str());
			
			unlink(filename.c_str());
			
			filenames.erase(filenames.begin());
		}
	}
	
	if (-1 < m_keep_size)
	{
		while(0 < filenames.size())
		{
			int free_size = GetStorageFreeSpace(m_path);
			if (free_size < 0)
			{
				break;
			}
			
			DBG_PRINT("free_size = %d / keep_size = %d", free_size, m_keep_size);
			if (m_keep_size <= free_size)
			{
				break;
			}
			
			std::string filename = *(filenames.begin());
			DBG_PRINT("erase by size: %s", filename.c_str());
			
			unlink(filename.c_str());
			
			filenames.erase(filenames.begin());
		}
	}
}

//---------------------------------------------------------------------------------------------------
