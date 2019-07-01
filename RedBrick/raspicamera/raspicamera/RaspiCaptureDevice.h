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

#ifndef RASPI_CAPTURE_DEVICE_H
#define RASPI_CAPTURE_DEVICE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sysexits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bcm_host.h"
#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

extern "C"
{

#include "RaspiCamControl.h"

}

#include "mhengine/MH_Event.h"
#include "mhmedia/MH_CaptureDevice.h"
#include "mhmedia/MH_MediaPacketPool.h"

/**
 * Structure containing all state information for the current run
 */
struct RASPIVID_STATE
{
   int width;                          /// Requested width of image
   int height;                         /// requested height of image
   MMAL_FOURCC_T encoding;             /// Requested codec video encoding (MJPEG or H264)
   int bitrate;                        /// Requested bitrate
   int framerate;                      /// Requested frame rate (fps)
   int intraperiod;                    /// Intra-refresh period (key frame rate)
   int quantisationParameter;          /// Quantisation parameter - quality. Set bitrate 0 and set this for variable bitrate
   int profile;                        /// H264 profile to use for encoding
   int level;                          /// H264 level to use for encoding

   RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters

   int cameraNum;                       /// Camera number
   int settings;                        /// Request settings from the camera
   int sensor_mode;			            /// Sensor mode. 0=auto. Check docs/forum for modes selected by other values.
   int intra_refresh_type;              /// What intra refresh type to use. -1 to not set.
};

class RaspiCaptureDevice : public MH_CaptureDevice
{
private:
	MMAL_PORT_T* camera_preview_port;
	MMAL_PORT_T* camera_video_port;
	MMAL_PORT_T* preview_input_port;
	MMAL_PORT_T* splitter_input_port;
	MMAL_PORT_T* splitter_output_port;
	MMAL_PORT_T* encoder_input_port;
	MMAL_PORT_T* encoder_output_port;

	MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
	MMAL_COMPONENT_T *splitter_component;  /// Pointer to the splitter component
	MMAL_COMPONENT_T *encoder_component;   /// Pointer to the encoder component
	MMAL_CONNECTION_T *splitter_connection;/// Pointer to the connection from camera to splitter
	MMAL_CONNECTION_T *encoder_connection; /// Pointer to the connection from camera to encoder

	MMAL_POOL_T *splitter_pool; /// Pointer to the pool of buffers used by splitter output port 0
	MMAL_POOL_T *encoder_pool; /// Pointer to the pool of buffers used by encoder output port
   
	unsigned char sps[29];
	int  sps_size;

	unsigned char pps[29];
	int  pps_size;
	
	MH_MediaPacketPool* m_packet_pool;

	MH_Event* m_snapshot_event;
	MH_MediaPacket* m_snapshot;
   
   int64_t m_first_time;
   int64_t m_first_pts;
   
   int m_max_packet_size;
   
	char m_annotation_text[80];

	MMAL_STATUS_T create_camera_component(RASPIVID_STATE *state);
	MMAL_STATUS_T create_splitter_component(RASPIVID_STATE *state);
	MMAL_STATUS_T create_encoder_component(RASPIVID_STATE *state);
	void destroy_camera_component();
	void destroy_splitter_component();
	void destroy_encoder_component();

	void update_annotation_data();

	void check_disable_port(MMAL_PORT_T *port);

	MMAL_STATUS_T connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection);

	static void camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	static void splitter_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
	void SplitterBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	static void encoder_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
	void EnoderBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	virtual bool OnInitialize();
	virtual void OnTerminate();
	
	virtual bool OnStart(CaptureParam* param);
	virtual void OnStop();
	
	virtual bool OnSnapShot(MH_MediaPacket* packet);

public:
	RaspiCaptureDevice();
	virtual ~RaspiCaptureDevice();
};

#endif
