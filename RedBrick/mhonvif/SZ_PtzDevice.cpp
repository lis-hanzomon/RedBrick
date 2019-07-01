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
#include "mhonvif/SZ_PtzDevice.h"

SZ_PtzDevice::SZ_PtzDevice()
{
}

SZ_PtzDevice::~SZ_PtzDevice()
{
}

bool SZ_PtzDevice::Initialize()
{
	m_x = 0;
	m_y = 0;
	
	if (!OnInitialize())
	{
		Terminate();
		return false;
	}
	
	return true;
}

bool SZ_PtzDevice::OnInitialize()
{
	return true;
}

void SZ_PtzDevice::Terminate()
{
	OnTerminate();
}

void SZ_PtzDevice::OnTerminate()
{
}

float SZ_PtzDevice::GetPosX()
{
	MH_AutoLock lock(&m_sync);
	
	return m_x;
}

float SZ_PtzDevice::GetPosY()
{
	MH_AutoLock lock(&m_sync);

	return m_y;
}

void SZ_PtzDevice::UpdatePos(float x, float y)
{
	MH_AutoLock lock(&m_sync);

	m_x = x;
	m_y = y;
}

bool SZ_PtzDevice::AbsoluteMove(float x, float y, float sx, float sy)
{
	MH_AutoLock lock(&m_sync);

	if (x < -1)
	{
		x = -1;
	}
	else if (1 < x)
	{
		x = 1;
	}
	
	if (y < -1)
	{
		y = -1;
	}
	else if (1 < y)
	{
		y = 1;
	}
	
	return OnAbsoluteMove(x, y, sx, sy);
}

bool SZ_PtzDevice::RelativeMove(float dx, float dy, float sx, float sy)
{
	MH_AutoLock lock(&m_sync);

	return AbsoluteMove(m_x + dx, m_y + dy, sx, sy);
}

bool SZ_PtzDevice::ContinuousMove(float sx, float sy)
{
	MH_AutoLock lock(&m_sync);

	float x = m_x;
	float y = m_y;
	
	if (sx < 0)
	{
		x = -1;
	}
	else if (0 < sx)
	{
		x = 1;
	}

	if (sy < 0)
	{
		y = -1;
	}
	else if (0 < sy)
	{
		y = 1;
	}
	
	return OnAbsoluteMove(x, y, sx, sy);
}

bool SZ_PtzDevice::OnAbsoluteMove(float x, float y, float sx, float sy)
{
	return false;
}

bool SZ_PtzDevice::StopMove()
{
	return OnStopMove();
}

bool SZ_PtzDevice::OnStopMove()
{
	return false;
}
