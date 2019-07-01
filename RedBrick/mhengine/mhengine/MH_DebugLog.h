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

#ifndef MH_DEBUG_LOG_H
#define MH_DEBUG_LOG_H

#include "MH_Engine.h"

#ifdef __cplusplus
extern "C" {
#endif

void MHENGINE_API DebugTrace(const char* file, int line, const char* msg);
void MHENGINE_API DebugPrint(const char* file, int line, const char* fmt, ...);
#ifdef WIN32
void MHENGINE_API DebugPrintW(const char* file, int line, const wchar_t* fmt, ...);
#endif
void MHENGINE_API DebugDump(const char* file, int line, const void* buff, int size);

#ifdef __cplusplus
}
#endif

#ifdef OUTPUT_DEBUG_LOG

#define DBG_TRACE(msg) DebugTrace(__FILE__, __LINE__, msg)
#define DBG_PRINT(...) DebugPrint(__FILE__, __LINE__, __VA_ARGS__)
#ifdef WIN32
#define DBG_PRINT_W(...) DebugPrintW(__FILE__, __LINE__, __VA_ARGS__)
#else
#define DBG_PRINT_W(...)
#endif
#define DBG_DUMP(buff, size)  DebugDump(__FILE__, __LINE__, buff, size)

#else

#define DBG_TRACE(msg)
#define DBG_PRINT(...)
#define DBG_PRINT_W(...)
#define DBG_DUMP(buff, size)

#endif

#endif
