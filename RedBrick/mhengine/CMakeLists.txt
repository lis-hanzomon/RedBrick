
cmake_minimum_required(VERSION 2.8)

include (CheckIncludeFiles)

check_include_files ("uuid/uuid.h" UUID_H)
if (NOT UUID_H)
	message(FATAL_ERROR "uuid.h not found - uuid-dev not installed?")
endif(NOT UUID_H)

file(GLOB SOURCES 
	MH_Engine.cpp 
	MH_NetInterface.cpp
	MH_CriticalSection.cpp
	MH_Event.cpp
	MH_Thread.cpp
	MH_MessageQueue.cpp
	MH_MessageQueueManager.cpp
	MH_MessageTask.cpp
	MH_Socket.cpp
	MH_ServerSocket.cpp
	MH_SocketTask.cpp
	MH_ServerSocketTask.cpp
	MH_UnixSocket.cpp
	MH_UnixServerSocket.cpp
	MH_UdpSocket.cpp)

add_library(mhengine SHARED ${SOURCES})

install(TARGETS mhengine LIBRARY DESTINATION lib)

file(GLOB HEADERS 
	mhengine/MH_Engine.h 
	mhengine/MH_NetInterface.h
	mhengine/MH_CriticalSection.h
	mhengine/MH_Event.h
	mhengine/MH_Thread.h
	mhengine/MH_MessageQueue.h
	mhengine/MH_MessageQueueManager.h
	mhengine/MH_MessageTask.h
	mhengine/MH_Socket.h
	mhengine/MH_ServerSocket.h
	mhengine/MH_UnixSocket.h
	mhengine/MH_UnixServerSocket.h
	mhengine/MH_UdpSocket.h
	mhengine/MH_DebugLog.h)

install(FILES ${HEADERS} DESTINATION include/mhengine)
