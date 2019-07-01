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
#include "SZ_NetworkSettingImpl.h"
#include "mhengine/MH_NetInterface.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <ifaddrs.h>
#include <arpa/inet.h>

#include <string>
#include <vector>

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

SZ_NetworkSettingImpl::SZ_NetworkSettingImpl()
{
	DBG_TRACE("Enter SZ_NetworkSettingImpl::SZ_NetworkSettingImpl");
	
	DBG_TRACE("Exit SZ_NetworkSettingImpl::SZ_NetworkSettingImpl");
}

SZ_NetworkSettingImpl::~SZ_NetworkSettingImpl()
{
	DBG_TRACE("Enter SZ_NetworkSettingImpl::~SZ_NetworkSettingImpl");
	
	DBG_TRACE("Exit SZ_NetworkSettingImpl::~SZ_NetworkSettingImpl");
}

bool SZ_NetworkSettingImpl::Initialize(const char* if_name)
{
	DBG_TRACE("Enter SZ_NetworkSettingImpl::Initialize");
	DBG_PRINT("if_name = %s", if_name);
	
	strcpy(m_if_name, if_name);
	
	DBG_TRACE("Exit SZ_NetworkSettingImpl::Initialize");
	
	return true;
}

bool SZ_NetworkSettingImpl::Load()
{
	DBG_TRACE("Enter SZ_NetworkSettingImpl::Load");
	
	//--------------------------------------------------
	// ホスト名を取得する
	//--------------------------------------------------
	
	gethostname(m_hostname, sizeof(m_hostname));
	DBG_PRINT("HOSTNAME : %s", m_hostname);
	
	//--------------------------------------------------
	// MACアドレスを取得
	//--------------------------------------------------

	strcpy(m_hw_address, "");

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		return false;
	}
	
	struct ifreq ifr;
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, m_if_name, IFNAMSIZ - 1);
	ioctl(fd, SIOCGIFHWADDR, &ifr);
	
	close(fd);
	
	sprintf(m_hw_address, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
         (unsigned char)ifr.ifr_hwaddr.sa_data[0],
         (unsigned char)ifr.ifr_hwaddr.sa_data[1],
         (unsigned char)ifr.ifr_hwaddr.sa_data[2],
         (unsigned char)ifr.ifr_hwaddr.sa_data[3],
         (unsigned char)ifr.ifr_hwaddr.sa_data[4],
         (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
	DBG_PRINT("MAC : %s", m_hw_address);
	
	//--------------------------------------------------
	// IPアドレスを取得する
	//--------------------------------------------------
	
	strcpy(m_ip_address, "");
	
	ifaddrs* ifa_list;
	int n = getifaddrs(&ifa_list);
	if (n != 0)
	{
		return false;
	}

	bool find = false;

	for(ifaddrs* ifa = ifa_list; ifa != NULL; ifa=ifa->ifa_next)
	{
		DBG_PRINT("%s", ifa->ifa_name);
		
		if (strcmp(m_if_name, ifa->ifa_name) == 0)
		{
			char addrstr[256];
			memset(addrstr, 0, sizeof(addrstr));
			char netmaskstr[256];
			memset(netmaskstr, 0, sizeof(netmaskstr));

			if (ifa->ifa_addr->sa_family == AF_INET)
			{
				inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,    addrstr,    sizeof(addrstr));
				inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr, netmaskstr, sizeof(netmaskstr));

				DBG_PRINT("     IPv4: %s netmask %s", addrstr, netmaskstr);
				
				strcpy(m_ip_address, addrstr);
				
				int mask = ((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr.s_addr;
				m_prefix_len = 0;
				for(int i = 0; i < 24; i++)
				{
					if ((mask & 1) == 0)
					{
						break;
					}
					mask >>= 1;
					m_prefix_len++;
				}
				DBG_PRINT("PrefixLen: %d", m_prefix_len);
				
				find = true;
				break;
			} 
		}
		
		DBG_PRINT("");
	}

	freeifaddrs(ifa_list);	

	if (!find)
	{
		DBG_PRINT("Warnning: Can't determind IP Address");
		return false;
	}

	//--------------------------------------------------
	// DNSを取得
	//--------------------------------------------------
	
	LoadDnsAddress();
	
	//--------------------------------------------------
	// デフォルトゲートウェイを取得
	//--------------------------------------------------
	
	LoadGwAddress();
	
	DBG_TRACE("Exit SZ_NetworkSettingImpl::Load");
	
	return true;
}

bool SZ_NetworkSettingImpl::LoadDnsAddress()
{
	strcpy(m_dns_address, "");
	
	FILE* fp = fopen("/etc/resolv.conf", "r");
	if (!fp)
	{
		return false;
	}
	
	int i = 0;
	
	char buf[1025];
	while (fgets(buf, 1024, fp))
	{
	    if (!i++) continue;			// Skip first tag message.
		
	    if (strncmp(buf, "nameserver", 10) == 0)
	    {
	    	char dummy[40];
			sscanf(buf, "%s %s", dummy, m_dns_address);
			DBG_PRINT("DNS ADDRESS: %s", m_dns_address);
			break;
	    }
	}
	
	fclose(fp);
	
	return true;
}

bool SZ_NetworkSettingImpl::LoadGwAddress()
{
	strcpy(m_gw_address, "");
	
	FILE* fp = fopen("/proc/net/route", "r");		// Open /proc/net/route.
	if (!fp)
	{
		return false;
	}

	int i = 0;
	
	char buf[1025];
	while (fgets(buf, 1024, fp))
	{
	    if (!i++) continue;			// Skip first tag message.
	    
		char dev[9], dest[9], gw[9], mask[9];
		int flags, cnt, use, met, mtu, win, irtt;
	    sscanf(buf, "%8s %8s %8s %d %d %d %d %8s %d %d %d", 
	    	dev, dest, gw, &flags, &cnt, &use, &met, mask, &mtu, &win, &irtt
	    );

#define RTF_UP		0x0001
#define RTF_GATEWAY	0x0002

		if (!(flags & RTF_UP)) continue;		// Route is not usable.

		if (flags & RTF_GATEWAY)				// This is a gateway.
		{
			int a1, a2, a3, a4;
			sscanf(gw, "%02X%02X%02X%02X", &a4, &a3, &a2, &a1);
			sprintf(m_gw_address, "%d.%d.%d.%d", a1, a2, a3, a4);
		}
	}
	fclose(fp);

	DBG_PRINT("GW ADDRESS: %s", m_gw_address);

	return true;
}

void SZ_NetworkSettingImpl::GetHostname(char* hostname)
{
	strcpy(hostname, m_hostname);
}

bool SZ_NetworkSettingImpl::SetHostname(const char* hostname)
{
	// 未対応
	return true;
}

const char* SZ_NetworkSettingImpl::GetAddress()
{
	return m_ip_address;
}

void SZ_NetworkSettingImpl::GetNetwork(char* if_name, char* hw_address, bool* use_dhcp, char* address, int* prefix_len)
{
	// DHCP動作を想定
	*use_dhcp = true;
	
	strcpy(if_name, m_if_name);
	strcpy(hw_address, m_hw_address);
	strcpy(address, m_ip_address);
	*prefix_len = m_prefix_len;
}

bool SZ_NetworkSettingImpl::SetNetwork(const char* if_name, bool use_dhcp, const char* address, int prefix_len)
{
	// 未対応
	return true;
}

void SZ_NetworkSettingImpl::GetDnsAddress(bool* use_dhcp, char* address)
{
	// DHCP動作を想定
	*use_dhcp = true;

	strcpy(address, m_dns_address);
}

bool SZ_NetworkSettingImpl::SetDnsAddress(bool use_dhcp, const char* address)
{
	// 未対応
	return true;
}

void SZ_NetworkSettingImpl::GetGatewayAddress(char* address)
{
	strcpy(address, m_gw_address);
}

bool SZ_NetworkSettingImpl::SetGatewayAddress(const char* address)
{
	// 未対応
	return true;
}
