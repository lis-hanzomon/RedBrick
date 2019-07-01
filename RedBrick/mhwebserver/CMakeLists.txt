
cmake_minimum_required(VERSION 2.8)

include_directories(${PROJECT_SOURCE_DIR}/mhengine)

file(GLOB SOURCES 
	mongoose.c
	MH_WebServer.cpp)

add_definitions(-DMG_ENABLE_IPV6)

add_library(mhwebserver SHARED ${SOURCES})

install(TARGETS mhwebserver LIBRARY DESTINATION lib)

file(GLOB HEADERS 
	mhwebserver/MH_WebServer.h)

install(FILES ${HEADERS} DESTINATION include/mhwebserver)
