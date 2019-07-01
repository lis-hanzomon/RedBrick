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

#ifndef SZ_PTZ_DEVICE_H
#define SZ_PTZ_DEVICE_H

#include "SZ17.h"

#include "mhengine/MH_CriticalSection.h"

/**
 * パン・チルド制御デバイス
 */
class SZ17_API SZ_PtzDevice
{
private:
	MH_CriticalSection m_sync;

	/**
	 * 現在の位置 (-1 <= 0 <= 1)
	 */
	float m_x;
	float m_y;
	
protected:
	/**
	 * 初期化
	 */
	virtual bool OnInitialize();
	
	/**
	 * 終了処理
	 */
	virtual void OnTerminate();
	
	/**
	 * 絶対座標移動
	 */
	virtual bool OnAbsoluteMove(float x, float y, float sx, float sy);
	
	/**
	 * 移動停止
	 */
	virtual bool OnStopMove();
	
	/**
	 * 現在位置更新
	 */
	void UpdatePos(float x, float y);

public:
	/**
	 * コンストラクタ
	 */
	SZ_PtzDevice();
	
	/**
	 * デストラクタ
	 */
	virtual ~SZ_PtzDevice();
	
	/**
	 * 初期化処理
	 */
	bool Initialize();
	
	/**
	 * 終了処理
	 */
	void Terminate();

	/**
	 * 現在位置取得
	 */
	float GetPosX();
	float GetPosY();
	
	/**
	 * 絶対座標移動
	 */
	virtual bool AbsoluteMove(float x, float y, float sx, float sy);
	
	/**
	 * 相対座標移動
	 */
	virtual bool RelativeMove(float dx, float dy, float sx, float sy);
	
	/**
	 * 連続移動
	 */
	virtual bool ContinuousMove(float sx, float sy);
	
	/**
	 * 移動停止
	 */
	virtual bool StopMove();
	
	/**
	 * 移動中取得
	 */
	virtual bool IsMoving() = 0;
};

#endif
