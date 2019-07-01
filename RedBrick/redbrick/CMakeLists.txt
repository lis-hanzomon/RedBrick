
cmake_minimum_required(VERSION 2.8)

include_directories(${PROJECT_SOURCE_DIR}/mhengine)
include_directories(${PROJECT_SOURCE_DIR}/mhmedia)
include_directories(${PROJECT_SOURCE_DIR}/mhdiscovery)
include_directories(${PROJECT_SOURCE_DIR}/mhwebserver)
include_directories(${PROJECT_SOURCE_DIR}/raspicamera)
include_directories(${PROJECT_SOURCE_DIR}/mhonvif)

include_directories(/opt/vc/include)
include_directories(/usr/local/include/BasicUsageEnvironment)
include_directories(/usr/local/include/UsageEnvironment)
include_directories(/usr/local/include/liveMedia)
include_directories(/usr/local/include/groupsock)

link_directories(/opt/vc/lib)

file(GLOB SOURCES 
	SZ_NetworkSettingImpl.cpp
	SZ_ProfileSettingImpl.cpp
	SZ_UserSettingImpl.cpp
	RaspiOnvifPlatform.cpp
	RaspiPtzDevice.cpp
	daemonize.cpp
	main.cpp
	)

add_executable(redbrick ${SOURCES})

set (MMAL_LIBS bcm_host mmal_core mmal_util mmal_vc_client)
set (FFMPEG_LIBS avformat avcodec avutil swscale swresample)
set (JSONC_LIBS json-c)

target_link_libraries(redbrick ${MMAL_LIBS} ${FFMPEG_LIBS} ${JSONC_LIBS} mhengine mhdiscovery mhmedia raspicamera mhwebserver mhonvif pthread rt uuid xml2)

install (TARGETS redbrick DESTINATION bin)

file(GLOB SETTINGS 
	settings/device_info.json
	settings/onvif_setting.json
	settings/profile.json
	settings/user_info.json
	settings/video_encoder_config.json)

install (FILES ${SETTINGS} DESTINATION /etc/redbrick)

install (FILES scripts/redbrick DESTINATION /etc/init.d PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
