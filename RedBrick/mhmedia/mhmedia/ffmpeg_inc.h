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

#ifndef FFMPEG_INC_H
#define FFMPEG_INC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include "libavutil/common.h"
#include "libavutil/avstring.h"
#ifdef ANDROID_OS
#include "libavutil/avtime.h"
#else
#include "libavutil/time.h"
#endif
#include "libavutil/samplefmt.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
	
#ifdef __cplusplus
}
#endif

#endif
