
cmake_minimum_required(VERSION 2.8)

check_include_files ("libavformat/avformat.h" LIBAVFORMAT_H)
if (NOT LIBAVFORMAT_H)
	message(FATAL_ERROR "avformat.h not found - libavformat-dev not installed?")
endif(NOT LIBAVFORMAT_H)

check_include_files ("libswscale/swscale.h" LIBAVSWSCALE_H)
if (NOT LIBAVSWSCALE_H)
	message(FATAL_ERROR "swscale.h not found - libswscale-dev not installed?")
endif(NOT LIBAVSWSCALE_H)

include_directories(${PROJECT_SOURCE_DIR}/mhengine)

include_directories(/usr/local/include/BasicUsageEnvironment)
include_directories(/usr/local/include/UsageEnvironment)
include_directories(/usr/local/include/liveMedia)
include_directories(/usr/local/include/groupsock)

file(GLOB SOURCES 
	MH_MediaPacket.cpp
	MH_MediaPacketPoolInterface.cpp
	MH_MediaPacketPool.cpp
	MH_MediaPacketSrc.cpp
	MH_MediaPacketSink.cpp
	MH_MediaPacketStream.cpp
	MH_FileSink.cpp
	MH_JpegWriter.cpp
	MH_MediaPacketFramedSource.cpp
	MH_VideoServerMediaSubsession.cpp
	MH_RtspServerThread.cpp
	MH_CaptureDevice.cpp
	MH_CameraDevice.cpp
	MH_MediaRecoder.cpp
	MH_Media.cpp)

add_library(mhmedia SHARED ${SOURCES})

set (LIVE_LIBS liveMedia BasicUsageEnvironment UsageEnvironment groupsock)

target_link_libraries(mhmedia ${LIVE_LIBS})

install(TARGETS mhmedia LIBRARY DESTINATION lib)

file(GLOB HEADERS 
	mhmedia/MH_MediaPacket.h
	mhmedia/MH_MediaPacketPoolInterface.h
	mhmedia/MH_MediaPacketPool.h
	mhmedia/MH_MediaPacketSrc.h
	mhmedia/MH_MediaPacketSink.h
	mhmedia/MH_MediaPacketStream.h
	mhmedia/MH_FileSink.h
	mhmedia/ffmpeg_inc.h
	mhmedia/MH_JpegWriter.h
	mhmedia/MH_JpegWriteThread.h
	mhmedia/MH_RtspServerThread.h
	mhmedia/MH_CaptureDevice.h
	mhmedia/MH_CameraDevice.h
	mhmedia/MH_MediaRecoder.h
	mhmedia/MH_Media.h)

install(FILES ${HEADERS} DESTINATION include/mhmedia)
