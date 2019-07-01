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

#ifndef SZ_NETWORK_SETTING_H
#define SZ_NETWORK_SETTING_H

#include "SZ17.h"

class SZ17_API SZ_NetworkSetting
{
public:
	SZ_NetworkSetting();
	virtual ~SZ_NetworkSetting();
	
	virtual bool Load() = 0;
	
	virtual const char* GetAddress() = 0;
	
	virtual bool SetHostname(const char* hostname) = 0;
	virtual void GetHostname(char* hostname) = 0;
	
	virtual bool SetNetwork(const char* if_name, bool use_dhcp, const char* address, int prefix_len) = 0;
	virtual void GetNetwork(char* if_name, char* hw_address, bool* use_dhcp, char* address, int* prefix_len) = 0;

	virtual bool SetDnsAddress(bool use_dhcp, const char* address) = 0;
	virtual void GetDnsAddress(bool* use_dhcp, char* address) = 0;

	virtual bool SetGatewayAddress(const char* address) = 0;
	virtual void GetGatewayAddress(char* address) = 0;
};

#endif
