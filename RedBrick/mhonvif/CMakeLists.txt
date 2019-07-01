
cmake_minimum_required(VERSION 2.8)

include_directories(${PROJECT_SOURCE_DIR}/mhengine)
include_directories(${PROJECT_SOURCE_DIR}/mhmedia)
include_directories(${PROJECT_SOURCE_DIR}/mhdiscovery)
include_directories(${PROJECT_SOURCE_DIR}/mhwebserver)

include_directories(/opt/vc/include)
include_directories(/usr/local/include/BasicUsageEnvironment)
include_directories(/usr/local/include/UsageEnvironment)
include_directories(/usr/local/include/liveMedia)
include_directories(/usr/local/include/groupsock)

file(GLOB SOURCES 
	SZ17.cpp
	soapC.c
	soapServer.c
	dom.c
	duration.c
	wsaapi.c
	smdevp.c
	mecevp.c
	wsseapi.c
	OnvifImpl.c
	OnvifBridge.cpp
	MotionDetect.cpp
	SnapShotWebServer.cpp
	ImageProcessThread.cpp
	OnvifService.cpp
	SZ_OnvifProcess.cpp
	SZ_OnvifPlatform.cpp
	SZ_NetworkSetting.cpp
	SZ_UserSetting.cpp
	SZ_ProfileSetting.cpp
	SZ_PtzDevice.cpp
	)

add_definitions(-DWITH_DOM)
add_definitions(-DWITH_OPENSSL)
add_definitions(-DWITH_NOSERVEREQUEST)
add_definitions(-DWITH_IPV6)

add_library(mhonvif SHARED ${SOURCES})

set (OPENCV_LIBS opencv_shape opencv_objdetect opencv_shape opencv_stitching opencv_objdetect opencv_superres opencv_videostab opencv_calib3d opencv_features2d opencv_highgui opencv_videoio opencv_imgcodecs opencv_video opencv_photo opencv_ml opencv_imgproc opencv_flann opencv_core)

target_link_libraries(mhonvif ${OPENCV_LIBS})

set (OPENSSL_LIBS ssl crypt)
set (GSOAP_LIBS gsoapssl)

target_link_libraries(mhonvif ${GSOAP_LIBS} ${OPENSSL_LIBS})

install(TARGETS mhonvif LIBRARY DESTINATION lib)

file(GLOB HEADERS 
	mhonvif/SZ17.h
	mhonvif/SZ_OnvifInfo.h
	mhonvif/SZ_NetworkSetting.h
	mhonvif/SZ_OnvifPlatform.h
	mhonvif/SZ_ProfileSetting.h
	mhonvif/SZ_UserSetting.h
	mhonvif/SZ_PtzDevice.h
	mhonvif/SZ_OnvifProcess.h)

install(FILES ${HEADERS} DESTINATION include/mhonvif)
