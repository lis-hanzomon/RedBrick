
cmake_minimum_required(VERSION 2.8)

include_directories(/opt/vc/include)

include_directories(${PROJECT_SOURCE_DIR}/mhengine)
include_directories(${PROJECT_SOURCE_DIR}/mhmedia)

file(GLOB SOURCES 
	RaspiCLI.c
	RaspiCamControl.c
	RaspiCaptureDevice.cpp
	RaspiCameraDevice.cpp)

add_library(raspicamera SHARED ${SOURCES})

install(TARGETS raspicamera LIBRARY DESTINATION lib)

file(GLOB HEADERS 
	raspicamera/RaspiCamControl.h
	raspicamera/RaspiCaptureDevice.h
	raspicamera/RaspiCameraDevice.h)

install(FILES ${HEADERS} DESTINATION include/raspicamera)
