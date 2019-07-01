
cmake_minimum_required(VERSION 2.8)

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

include_directories(${PROJECT_SOURCE_DIR}/mhengine)

file(GLOB SOURCES 
	MH_Discovery.cpp
	MH_DiscoveryParser.cpp 
	MH_DiscoveryThread.cpp
	MH_DiscoveryServer.cpp
	MH_DiscoveryClient.cpp)

add_library(mhdiscovery SHARED ${SOURCES})

install(TARGETS mhdiscovery LIBRARY DESTINATION lib)

file(GLOB HEADERS 
	mhdiscovery/MH_Discovery.h
	mhdiscovery/MH_DiscoveryThread.h
	mhdiscovery/MH_DiscoveryServer.h
	mhdiscovery/MH_DiscoveryClient.h)

install(FILES ${HEADERS} DESTINATION include/mhdiscovery)
