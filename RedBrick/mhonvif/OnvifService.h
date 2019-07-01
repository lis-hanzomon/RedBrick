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

#ifndef ONVIF_SERVICE_H
#define ONVIF_SERVICE_H

#include "mhengine/MH_Thread.h"

class OnvifService : public MH_Thread
{
private:
	unsigned short m_port_no;

protected:
	virtual void OnMain();
	virtual void OnReqTerminate();

public:
	OnvifService();
	virtual ~OnvifService();
	
	virtual bool Start(unsigned short port);
};

#endif
