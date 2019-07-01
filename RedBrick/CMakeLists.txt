
cmake_minimum_required(VERSION 2.8)

project(RedBrick)

ADD_SUBDIRECTORY(mhengine)
ADD_SUBDIRECTORY(mhmedia)
ADD_SUBDIRECTORY(raspicamera)
ADD_SUBDIRECTORY(mhdiscovery)
ADD_SUBDIRECTORY(mhwebserver)
ADD_SUBDIRECTORY(mhonvif)
ADD_SUBDIRECTORY(redbrick)

include (InstallRequiredSystemLibraries)

set (CPACK_GENERATOR "DEB")
set (CPACK_PACKAGE_CONTACT "Koichi Tsubai <tsubai@lis.co.jp>") 
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set (CPACK_PACKAGE_VERSION_MAJOR "1")
set (CPACK_PACKAGE_VERSION_MINOR "0")

set (CPACK_DEBIAN_PACKAGE_DEPENDS "libxml2, libuuid1, libavformat57, libavcodec57, libavutil55, libavresample3, libswscale4, libssl1.1 libjson-c3 insserv")

configure_file (postinst.in postinst @ONLY)
configure_file (prerm.in prerm @ONLY)
set (CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "postinst;prerm")

include (CPack)
