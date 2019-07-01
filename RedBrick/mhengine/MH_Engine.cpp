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
#include "mhengine/MH_Engine.h"
#include "mhengine/MH_CriticalSection.h"

#ifndef WIN32
#include <signal.h>
#endif

#define USE_PRINTF
// #define USE_CRT_DBG_REPORT
// #define USE_ANDROID_LOG_WRITE

#define LOG_LINE_MAX 8192

#ifndef WIN32

void GetLocalTime(SYSTEMTIME* lpSystemTime)
{
	time_t t = time(NULL);
	struct tm t2;
 	localtime_r(&t, &t2);
	
	lpSystemTime->wYear         = t2.tm_year + 1900;
	lpSystemTime->wMonth        = t2.tm_mon + 1;
	lpSystemTime->wDayOfWeek    = 0;
	lpSystemTime->wDay          = t2.tm_mday;
	lpSystemTime->wHour         = t2.tm_hour;
	lpSystemTime->wMinute       = t2.tm_min;
	lpSystemTime->wSecond       = t2.tm_sec;
	lpSystemTime->wMilliseconds = 0;
}

#endif

static FILE* log_fp = NULL;

static void DebugOpen(const char* filename)
{
	if (filename == NULL)
	{
		return;
	}
	if (strlen(filename) == 0)
	{
		return;
	}
	if (log_fp != NULL)
	{
		return;
	}

	log_fp = fopen(filename, "w");
}

static void DebugClose()
{
	if (log_fp != NULL)
	{
		fclose(log_fp);
		log_fp = NULL;
	}
}

extern "C"
{

void DebugTrace(const char* file, int line, const char* msg)
{
	char buff[LOG_LINE_MAX];

	SYSTEMTIME curr_time;
	GetLocalTime(&curr_time);

#ifdef WIN32
	const char* filename = strrchr(file, '\\');
#else
	const char* filename = strrchr(file, '/');
#endif
	if (filename != NULL)
	{
		filename ++;
	}
	else
	{
		filename = file;
	}

	snprintf(
		buff,
		LOG_LINE_MAX,
#ifdef WIN32
		"%d:%d:%d(%d) - [%d] : %s - %d : %s",
		curr_time.wHour,
		curr_time.wMinute,
		curr_time.wSecond,
		GetTickCount(),
		GetCurrentThreadId(),
#else
		"%02d:%02d:%02d(%d) - [%ld] : %s - %d : %s",
		curr_time.wHour,
		curr_time.wMinute,
		curr_time.wSecond,
		(int)(clock() / (CLOCKS_PER_SEC / 1000)),
		pthread_self(),
#endif
		filename,
		line,
		msg
		);

#ifdef USE_PRINTF
	printf("%s\n", buff);
#endif

#ifdef USE_CRT_DBG_REPORT
	_CrtDbgReport(_CRT_WARN, NULL, NULL, NULL, "%s\n", buff);
#endif

#ifdef USE_ANDROID_LOG_WRITE
	__android_log_write(ANDROID_LOG_DEBUG, "", buff);
#endif

	if (log_fp != NULL)
	{
		fprintf(log_fp, "%s\n", buff);
		fflush(log_fp);
	}
}

void DebugPrint(const char* file, int line, const char* fmt, ...)
{
	va_list arg;
	char msg[LOG_LINE_MAX];

	va_start(arg, fmt);
	vsnprintf(msg, LOG_LINE_MAX, fmt, arg);
	va_end(arg);

	DebugTrace(file, line, msg);
}

#ifdef WIN32

void DebugPrintW(const char* file, int line, const wchar_t* fmt, ...)
{
	va_list arg;
	wchar_t msg[LOG_LINE_MAX];

	va_start(arg, fmt);
	wvsprintf(msg, fmt, arg);
	va_end(arg);

	char buff[LOG_LINE_MAX];
	if (WideCharToMultiByte(CP_THREAD_ACP, 0, msg, -1, buff, LOG_LINE_MAX, NULL, NULL) > 0)
	{
		DebugTrace(file, line, buff);
	}
}

#endif

void DebugDump(const char* file, int line, const void* buff, int size)
{
	const unsigned char* data = (const unsigned char*)buff;

	for (int i = 0; i < size; i += 16)
	{
		char msg[80];
		sprintf(msg, "%04X : ", i);
		for (int j = 0; j < 16 && i + j < size; j++)
		{
			char data_str[4];
			sprintf(data_str, "%02X ", *(data + i + j));
			strcat(msg, data_str);
		}
		DebugTrace(file, line, msg);
	}
}

}

void MH_Initialize(const char* filename)
{
	DebugOpen(filename);

#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
	signal(SIGPIPE, SIG_IGN);
#endif
}

void MH_Terminate()
{
#ifdef WIN32
	WSACleanup();
#endif

	DebugClose();
}

#include <sys/statfs.h>

int GetStorageFreeSpace(const char* mount_path)
{
    struct statfs buf;
	int rc = statfs(mount_path, &buf);
    if(rc < 0)
    {
        return -1;
	}
	return (int)(((long long)buf.f_bsize * (long long)buf.f_bfree) / 1000 / 1000);
}

#include <dirent.h>
#include <sys/stat.h>

void GetFiles(const char* path, std::list<std::string>* filenames)
{
	DIR* dp = opendir(path);
	if (NULL == dp)
	{
		return;
	}
	
	int fileNum = 0;
	int64_t fileSize = 0;
	
    dirent* dent = readdir(dp);
    while (NULL != dent) {
		char filePath[FILENAME_MAX];
		sprintf(filePath, "%s/%s", path, dent->d_name);
		struct stat st;
		if (0 == stat(filePath, &st))
		{
			if (S_ISREG(st.st_mode))
			{
				filenames->push_back(filePath);
			}
		}
		
        dent = readdir(dp);
    }
    
    filenames->sort();
}

#include <uuid/uuid.h>

void UuidGenerate(char* uuid)
{
	uuid_t u;
	uuid_generate(u);
	uuid_unparse_upper(u, uuid);
}
