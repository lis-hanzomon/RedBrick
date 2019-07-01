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
#include "mhmedia/MH_FileSink.h"

MH_FileSink::MH_FileSink()
	: m_fp(NULL)
{
}

MH_FileSink::~MH_FileSink()
{
}

bool MH_FileSink::Open(const char* filename)
{
	Close();
	
	m_fp = fopen(filename, "w");
	if (m_fp == NULL)
	{
		return false;
	}
	
	return true;
}

void MH_FileSink::Close()
{
	if (m_fp == NULL)
	{
		return;
	}
	fclose(m_fp);
	m_fp = NULL;
}

void MH_FileSink::Push(MH_MediaPacket* packet)
{
	if (m_fp == NULL)
	{
		return;
	}
	
	fwrite(packet->m_data, 1, packet->m_size, m_fp);
}
