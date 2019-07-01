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
#include "mhengine/MH_NetInterface.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#ifdef WIN32

#include <iptypes.h>
#include <iphlpapi.h>

void EnumIPv4AddressList(std::vector<std::string>& ip_address_list)
{
	ULONG ulOutBufLen = 0;
	GetAdaptersInfo(NULL, &ulOutBufLen);

	PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
	DWORD dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	if (dwRetVal == NO_ERROR)
	{
		for (PIP_ADAPTER_INFO pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next)
		{
			ip_address_list.push_back(pAdapter->IpAddressList.IpAddress.String);
		}
	}
	free(pAdapterInfo);
}

#else

#include <ifaddrs.h>
#include <arpa/inet.h>

void EnumIPv4AddressList(std::vector<std::string>& ip_address_list)
{
	ifaddrs* ifa_list;
	int n = getifaddrs(&ifa_list);
	if (n != 0)
	{
		return;
	}

	for(ifaddrs* ifa = ifa_list; ifa != NULL; ifa=ifa->ifa_next)
	{
		char addrstr[256];
		memset(addrstr, 0, sizeof(addrstr));
		char netmaskstr[256];
		memset(netmaskstr, 0, sizeof(netmaskstr));

		if (ifa->ifa_addr->sa_family == AF_INET)
		{
			inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,    addrstr,    sizeof(addrstr));
			inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr, netmaskstr, sizeof(netmaskstr));
			ip_address_list.push_back(addrstr);
		} 
		else if (ifa->ifa_addr->sa_family == AF_INET6)
		{
			inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr,    addrstr,    sizeof(addrstr));
			inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr, netmaskstr, sizeof(netmaskstr));
		}
	}

	freeifaddrs(ifa_list);
}

#endif
