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
 * �p���E�`���h����f�o�C�X
 */
class SZ17_API SZ_PtzDevice
{
private:
	MH_CriticalSection m_sync;

	/**
	 * ���݂̈ʒu (-1 <= 0 <= 1)
	 */
	float m_x;
	float m_y;
	
protected:
	/**
	 * ������
	 */
	virtual bool OnInitialize();
	
	/**
	 * �I������
	 */
	virtual void OnTerminate();
	
	/**
	 * ��΍��W�ړ�
	 */
	virtual bool OnAbsoluteMove(float x, float y, float sx, float sy);
	
	/**
	 * �ړ���~
	 */
	virtual bool OnStopMove();
	
	/**
	 * ���݈ʒu�X�V
	 */
	void UpdatePos(float x, float y);

public:
	/**
	 * �R���X�g���N�^
	 */
	SZ_PtzDevice();
	
	/**
	 * �f�X�g���N�^
	 */
	virtual ~SZ_PtzDevice();
	
	/**
	 * ����������
	 */
	bool Initialize();
	
	/**
	 * �I������
	 */
	void Terminate();

	/**
	 * ���݈ʒu�擾
	 */
	float GetPosX();
	float GetPosY();
	
	/**
	 * ��΍��W�ړ�
	 */
	virtual bool AbsoluteMove(float x, float y, float sx, float sy);
	
	/**
	 * ���΍��W�ړ�
	 */
	virtual bool RelativeMove(float dx, float dy, float sx, float sy);
	
	/**
	 * �A���ړ�
	 */
	virtual bool ContinuousMove(float sx, float sy);
	
	/**
	 * �ړ���~
	 */
	virtual bool StopMove();
	
	/**
	 * �ړ����擾
	 */
	virtual bool IsMoving() = 0;
};

#endif
