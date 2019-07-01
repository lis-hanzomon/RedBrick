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

#ifndef SZ17_H
#define SZ17_H

#ifdef WIN32

#ifdef SZ17_H_EXPORTS
#define SZ17_API __declspec(dllexport)
#else
#define SZ17_API __declspec(dllimport)
#endif

#else

#define SZ17_API

#endif

bool SZ17_API SZ17_Initialize(const char* filename);
void SZ17_API SZ17_Terminate();

#endif
