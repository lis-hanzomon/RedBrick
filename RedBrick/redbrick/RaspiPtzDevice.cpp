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
#include "RaspiPtzDevice.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <unistd.h>

#define PCA9685_SUBADR1 0x2
#define PCA9685_SUBADR2 0x3
#define PCA9685_SUBADR3 0x4

#define PCA9685_MODE1 0x0
#define PCA9685_PRESCALE 0xFE

#define LED0_ON_L 0x6
#define LED0_ON_H 0x7
#define LED0_OFF_L 0x8
#define LED0_OFF_H 0x9

#define ALLLED_ON_L 0xFA
#define ALLLED_ON_H 0xFB
#define ALLLED_OFF_L 0xFC
#define ALLLED_OFF_H 0xFD

//----------------------------------------------------------------------------------------

RaspiPtzDevice::RaspiPtzDevice()
	: m_moving_x(false)
	, m_moving_y(false)
	, m_destX(0)
	, m_destY(0)
	, m_deltaX(0)
	, m_deltaY(0)
	, m_event()
	, m_i2c(-1)
{
}

RaspiPtzDevice::~RaspiPtzDevice()
{
}

bool RaspiPtzDevice::OnInitialize()
{
	DBG_TRACE("Enter RaspiPtzDevice::OnInitialize");
	
	const char *i2cFileName = "/dev/i2c-1";
	int driverAddress = 0x40;
 
	if ((m_i2c = open(i2cFileName, O_RDWR)) < 0)
	{
		return false;
	}
	
	if (ioctl(m_i2c, I2C_SLAVE, driverAddress) < 0)
	{
		return false;
	}
	
	reset();
	
	setPWMFreq(60);
	
	setPWM(1, 0, CalcPulseWidth(90));
	setPWM(0, 0, CalcPulseWidth(90));
	
	m_moving_x = false;
	m_moving_y = false;
	
	if (!m_event.Create())
	{
		return false;
	}
	
	if (!Start())
	{
		return false;
	}
	
	DBG_TRACE("Exit RaspiPtzDevice::OnInitialize");

	return true;
}

void RaspiPtzDevice::OnTerminate()
{
	DBG_TRACE("Enter RaspiPtzDevice::OnTerminate");
	
	Stop();
	
	m_event.Close();
	
	if (m_i2c > 0)
	{
		close(m_i2c);
		m_i2c = -1;
	}
	
	DBG_TRACE("Exit RaspiPtzDevice::OnTerminate");
}

//----------------------------------------------------------------------------------------

void RaspiPtzDevice::write8(unsigned char addr, unsigned char d)
{
	unsigned char sendData[2];

	sendData[0] = addr;
	sendData[1] = d;
	if (write(m_i2c, sendData, 2) != 2)
	{
		DBG_PRINT("Error: write8");
	}
}

unsigned char RaspiPtzDevice::read8(unsigned char addr)
{
	unsigned char sendData;
	unsigned char readData;
 
	sendData = addr;
 	if (write(m_i2c, &sendData, 1) != 1)
	{
		DBG_PRINT("Error: read8");
	}
	else
	{
		if (read(m_i2c, &readData, 1) != 1)
		{
			DBG_PRINT("Error: read8");
		}
	}
 
	return readData;
}

void RaspiPtzDevice::reset(void)
{
	write8(PCA9685_MODE1, 0x0);
}

void RaspiPtzDevice::setPWMFreq(float freq)
{
	float prescaleval = 25000000;
 
	prescaleval /= 4096;
	prescaleval /= freq;
	prescaleval -= 1;
 
	unsigned char prescale = floor(prescaleval + 0.5);
 
	unsigned char oldmode = read8(PCA9685_MODE1);
	unsigned char newmode = (oldmode&0x7F) | 0x10; 
	write8(PCA9685_MODE1, newmode); 
	write8(PCA9685_PRESCALE, prescale);
	write8(PCA9685_MODE1, oldmode);
	usleep(5 * 1000);
	write8(PCA9685_MODE1, oldmode | 0xa1);  
}

void RaspiPtzDevice::setPWM(unsigned char srvNo, unsigned short onTime, unsigned short offTime)
{
	unsigned char sendData[5];
 
	sendData[0] = LED0_ON_L + 4 * srvNo;
	sendData[1] = (unsigned char)(0x00ff & onTime);
	sendData[2] = (unsigned char)((0xff00 & onTime) >> 8);
	sendData[3] = (unsigned char)(0x00ff & offTime);
	sendData[4] = (unsigned char)((0xff00 & offTime) >> 8);
 
	if (write(m_i2c, sendData, 5) != 5)
	{
		DBG_PRINT("Error: setPWM");
	}
}

unsigned short RaspiPtzDevice::CalcPulseWidth(int angle)
{
	return (650 - 150) * angle / 180 + 150;
}

//----------------------------------------------------------------------------------------

void RaspiPtzDevice::OnMain()
{
	DBG_TRACE("Enter RaspiPtzDevice::OnMain");
	
	while(!IsTerminate())
	{
		m_event.Wait();
		
		while((m_moving_x || m_moving_y) && !IsTerminate())
		{
			float x = GetPosX();
			float y = GetPosY();
			
			if (m_deltaX > 0)
			{
				x += m_deltaX;
				if (m_destX < x)
				{
					x = m_destX;
					m_moving_x = false;
				}
			}
			else if (m_deltaX < 0)
			{
				x += m_deltaX;
				if (x < m_destX)
				{
					x = m_destX;
					m_moving_x = false;
				}
			}
			
			if (m_deltaY > 0)
			{
				y += m_deltaY;
				if (m_destY < y)
				{
					y = m_destY;
					m_moving_y = false;
				}
			}
			else if (m_deltaY < 0)
			{
				y += m_deltaY;
				if (y < m_destY)
				{
					y = m_destY;
					m_moving_y = false;
				}
			}
			
			// -1 ～ 1 の値を 45度 ～ 135度 に変換する
			// ※ 90度が中心
			int x_angle = 90 - x * 45;
			int y_angle = 90 - y * 45;
			
			setPWM(1, 0, CalcPulseWidth(x_angle));
			setPWM(0, 0, CalcPulseWidth(y_angle));
			
			UpdatePos(x, y);
			
			if (!m_moving_x && !m_moving_y)
			{
				break;
			}
			
			m_event.Wait(50);
		}
	}
	
	DBG_TRACE("Exit RaspiPtzDevice::OnMain");
}

bool RaspiPtzDevice::OnAbsoluteMove(float x, float y, float sx, float sy)
{
	DBG_TRACE("Enter RaspiPtzDevice::OnAbsoluteMove");
	DBG_PRINT("sx = %f", sx);
	DBG_PRINT("sy = %f", sy);
	
	m_destX = x;
	m_destY = y;
	
	// 実測の結果、今回使用していたサーボが1秒間に45度と言う速度で動作している。
	m_deltaX = sx / 5.0;
	m_deltaY = sy / 5.0;
	
	m_moving_x = true;
	m_moving_y = true;
	
	m_event.Set();
	
	DBG_TRACE("Exit RaspiPtzDevice::OnAbsoluteMove");
	
	return true;
}

bool RaspiPtzDevice::OnStopMove()
{
	DBG_TRACE("Enter RaspiPtzDevice::OnStopMove");

	m_moving_x = false;
	m_moving_y = false;
	
	m_event.Set();
	
	DBG_TRACE("Exit RaspiPtzDevice::OnStopMove");
	
	return true;
}

bool RaspiPtzDevice::IsMoving()
{
	if (m_moving_x || m_moving_y)
	{
		return true;
	}
	return false;
}

void RaspiPtzDevice::OnReqTerminate()
{
	DBG_TRACE("Enter RaspiPtzDevice::OnReqTerminate");
	
	m_event.Set();
	
	DBG_TRACE("Exit RaspiPtzDevice::OnReqTerminate");
}

//----------------------------------------------------------------------------------------
