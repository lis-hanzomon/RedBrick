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

#ifndef SZ_NETWORK_SETTING_IMPL_H
#define SZ_NETWORK_SETTING_IMPL_H

#include "mhonvif/SZ_NetworkSetting.h"

class SZ_NetworkSettingImpl : public SZ_NetworkSetting
{
private:
	char m_hostname[80];

	char m_if_name[80];
	char m_hw_address[80];
	char m_ip_address[80];
	int m_prefix_len;
	
	char m_dns_address[80];
	char m_gw_address[80];

	bool LoadDnsAddress();
	bool LoadGwAddress();
	
public:
	SZ_NetworkSettingImpl();
	virtual ~SZ_NetworkSettingImpl();
	
	bool Initialize(const char* if_name);
	
	virtual bool Load();

	virtual bool SetHostname(const char* hostname);
	virtual void GetHostname(char* hostname);
	
	virtual const char* GetAddress();
	
	virtual bool SetNetwork(const char* if_name, bool use_dhcp, const char* address, int prefix_len);
	virtual void GetNetwork(char* if_name, char* hw_address, bool* use_dhcp, char* address, int* prefix_len);

	virtual bool SetDnsAddress(bool use_dhcp, const char* address);
	virtual void GetDnsAddress(bool* use_dhcp, char* address);

	virtual bool SetGatewayAddress(const char* address);
	virtual void GetGatewayAddress(char* address);
};

#endif
