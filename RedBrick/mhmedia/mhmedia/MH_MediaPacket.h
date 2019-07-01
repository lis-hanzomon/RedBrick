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

#ifndef MH_MEDIA_PACKET_H
#define MH_MEDIA_PACKET_H

#include "MH_Media.h"

#include "MH_MediaPacketPoolInterface.h"

/**
 * �p�P�b�g�^�C�v
 */
const int TYPE_OTHER_DATA = 0;
const int TYPE_AUDIO_DATA = 1;
const int TYPE_VIDEO_DATA = 2;

/**
 * ������PTS
 */
const long long PTS_NO_VALUE = (long long)0x8000000000000000LL;

/**
 * ���f�B�A�p�P�b�g
 */
class MHMEDIA_API MH_MediaPacket
{
private:
	/**
	 * �e�p�P�b�g�v�[��
	 */
	MH_MediaPacketPoolInterface* m_pool;

	/**
	 * �Q�ƃJ�E���^
	 */
	int m_ref_count;

	/**
	 * �e�p�P�b�g�v�[���ł͂Ȃ��A�������g�Ń����������蓖�Ă�
	 */
	bool m_expand;

	/**
	 * �L�^���e���Z�b�g
	 */
	void Reset();

public:
	int m_id;					// �p�P�b�gID

	int m_type;					// �p�P�b�g�^�C�v

	bool m_key_frame;			// �L�[�t���[��

	long long m_pts;			// PTS
	long long m_dts;			// DTS

	unsigned char* m_buffer;	// �o�b�t�@
	int m_buffer_size;			// �o�b�t�@�T�C�Y
	
	unsigned char* m_data;		// �L���f�[�^
	int m_size;					// �L���f�[�^�T�C�Y

	int m_duration;				// �Đ�����

	/**
	 * ���������
	 */
	void Free();

public:
	/**
	 * �R���X�g���N�^
	 */
	MH_MediaPacket(MH_MediaPacketPoolInterface* pool);

	/**
	 * �R���X�g���N�^
	 */
	MH_MediaPacket(MH_MediaPacketPoolInterface* pool, unsigned char* data, int data_size);

	/**
	 * �f�X�g���N�^
	 */
	~MH_MediaPacket();

	/**
	 * ���������蓖��
	 */
	bool Allocate(int data_size);

	/**
	 * �Q�ƃJ�E���^�ǉ�
	 */
	void AddRef();

	/**
	 * �Q�ƃJ�E���^�폜
	 */
	void ReleaseRef();

	/**
	 * �f�[�^�i�[
	 */
	bool Store(unsigned char* data, int data_size);

	friend class MH_MediaPacketPoolInterface;
};

#endif
