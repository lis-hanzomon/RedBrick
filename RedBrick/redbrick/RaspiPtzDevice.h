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

#ifndef RASPI_PTZ_DEVICE_H
#define RASPI_PTZ_DEVICE_H

#include "mhengine/MH_Event.h"
#include "mhengine/MH_Thread.h"
#include "mhonvif/SZ_PtzDevice.h"

class RaspiPtzDevice : public SZ_PtzDevice
					 , public MH_Thread
{
private:
	bool m_moving_x;
	bool m_moving_y;
	
	float m_destX;
	float m_destY;
	float m_deltaX;
	float m_deltaY;
	
	MH_Event m_event;
	
	int m_i2c;

	unsigned char read8(unsigned char addr);
	void write8(unsigned char addr, unsigned char d);
	void reset(void);
	void setPWMFreq(float frea);
	void setPWM(unsigned char srvNo, unsigned short onTime, unsigned short offTime);

	unsigned short CalcPulseWidth(int angle);

protected:
	virtual bool OnInitialize();
	virtual void OnTerminate();
	
	virtual bool OnAbsoluteMove(float x, float y, float sx, float sy);
	virtual bool OnStopMove();
	
	virtual void OnMain();
	virtual void OnReqTerminate();
	
public:
	RaspiPtzDevice();
	virtual ~RaspiPtzDevice();
	
	virtual bool IsMoving();
};

#endif
