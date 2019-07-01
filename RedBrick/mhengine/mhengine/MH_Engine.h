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

#ifndef MH_ENGINE_H
#define MH_ENGINE_H

#ifdef WIN32

#ifdef MHENGINE_EXPORTS
#define MHENGINE_API __declspec(dllexport)
#else
#define MHENGINE_API __declspec(dllimport)
#endif

#else

#define MHENGINE_API

#endif

void MHENGINE_API MH_Initialize(const char* filename);
void MHENGINE_API MH_Terminate();

#ifndef WIN32

typedef unsigned short WORD;

typedef struct _SYSTEMTIME {
   WORD wYear;
   WORD wMonth;
   WORD wDayOfWeek;
   WORD wDay;
   WORD wHour;
   WORD wMinute;
   WORD wSecond;
   WORD wMilliseconds;
} SYSTEMTIME;

void MHENGINE_API GetLocalTime(SYSTEMTIME* lpSystemTime);

#endif

int MHENGINE_API GetStorageFreeSpace(const char* mount_path);

#ifdef __cplusplus

#include <list>
#include <string>

void MHENGINE_API GetFiles(const char* path, std::list<std::string>* filenames);

#endif

void MHENGINE_API UuidGenerate(char* uuid);

#endif
