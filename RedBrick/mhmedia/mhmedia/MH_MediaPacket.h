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
 * パケットタイプ
 */
const int TYPE_OTHER_DATA = 0;
const int TYPE_AUDIO_DATA = 1;
const int TYPE_VIDEO_DATA = 2;

/**
 * 無効なPTS
 */
const long long PTS_NO_VALUE = (long long)0x8000000000000000LL;

/**
 * メディアパケット
 */
class MHMEDIA_API MH_MediaPacket
{
private:
	/**
	 * 親パケットプール
	 */
	MH_MediaPacketPoolInterface* m_pool;

	/**
	 * 参照カウンタ
	 */
	int m_ref_count;

	/**
	 * 親パケットプールではなく、自分自身でメモリを割り当てた
	 */
	bool m_expand;

	/**
	 * 記録内容リセット
	 */
	void Reset();

public:
	int m_id;					// パケットID

	int m_type;					// パケットタイプ

	bool m_key_frame;			// キーフレーム

	long long m_pts;			// PTS
	long long m_dts;			// DTS

	unsigned char* m_buffer;	// バッファ
	int m_buffer_size;			// バッファサイズ
	
	unsigned char* m_data;		// 有効データ
	int m_size;					// 有効データサイズ

	int m_duration;				// 再生時間

	/**
	 * メモリ解放
	 */
	void Free();

public:
	/**
	 * コンストラクタ
	 */
	MH_MediaPacket(MH_MediaPacketPoolInterface* pool);

	/**
	 * コンストラクタ
	 */
	MH_MediaPacket(MH_MediaPacketPoolInterface* pool, unsigned char* data, int data_size);

	/**
	 * デストラクタ
	 */
	~MH_MediaPacket();

	/**
	 * メモリ割り当て
	 */
	bool Allocate(int data_size);

	/**
	 * 参照カウンタ追加
	 */
	void AddRef();

	/**
	 * 参照カウンタ削除
	 */
	void ReleaseRef();

	/**
	 * データ格納
	 */
	bool Store(unsigned char* data, int data_size);

	friend class MH_MediaPacketPoolInterface;
};

#endif
