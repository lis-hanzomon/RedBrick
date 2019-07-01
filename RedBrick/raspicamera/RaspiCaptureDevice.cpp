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

#include "stdafx.h"
#include "raspicamera/RaspiCaptureDevice.h"

#define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#include <sys/time.h>

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

// Port configuration for the splitter component
#define SPLITTER_OUTPUT_PORT 0
#define SPLITTER_PREVIEW_PORT 1

// Video format information
// 0 implies variable
#define VIDEO_FRAME_RATE_NUM 60
#define VIDEO_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

// Max bitrate we allow for recording
const int MAX_BITRATE_MJPEG = 25000000; // 25Mbits/s
const int MAX_BITRATE_LEVEL4 = 25000000; // 25Mbits/s
const int MAX_BITRATE_LEVEL42 = 62500000; // 62.5Mbits/s

RaspiCaptureDevice::RaspiCaptureDevice()
	: camera_preview_port(NULL)
	, camera_video_port(NULL)
	, splitter_input_port(NULL)
	, splitter_output_port(NULL)
	, encoder_input_port(NULL)
	, encoder_output_port(NULL)
	, camera_component(NULL)
	, splitter_component(NULL)
	, encoder_component(NULL)
	, splitter_connection(NULL)
	, encoder_connection(NULL)
	, splitter_pool(NULL)
	, encoder_pool(NULL)
	, sps_size(0)
	, pps_size(0)
	, m_packet_pool(NULL)
	, m_snapshot_event(NULL)
	, m_snapshot(NULL)
	, m_first_time(-1)
	, m_first_pts(-1)
	, m_max_packet_size(0)
{
	DBG_TRACE("Enter RaspiCaptureDevice::RaspiCaptureDevice");
	
	DBG_TRACE("Exit RaspiCaptureDevice::RaspiCaptureDevice");
}

RaspiCaptureDevice::~RaspiCaptureDevice()
{
	DBG_TRACE("Enter RaspiCaptureDevice::~RaspiCaptureDevice");
	
	DBG_TRACE("Exit RaspiCaptureDevice::~RaspiCaptureDevice");
}

bool RaspiCaptureDevice::OnInitialize()
{
	DBG_TRACE("Enter RaspiCaptureDevice::OnInitialize");
	
	bcm_host_init();
	
	DBG_TRACE("Exit RaspiCaptureDevice::OnInitialize");
	
	return true;
}

void RaspiCaptureDevice::OnTerminate()
{
}

MMAL_STATUS_T RaspiCaptureDevice::create_camera_component(RASPIVID_STATE *state)
{
	DBG_TRACE("Enter RaspiCaptureDevice::create_camera_component");

	MMAL_COMPONENT_T *camera = 0;
	MMAL_ES_FORMAT_T *format;
	MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
	MMAL_STATUS_T status;

	do
	{
	   /* Create the component */
	 	DBG_PRINT("--- mmal_component_create ---");
		status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
	   if (status != MMAL_SUCCESS)
	   {
			DBG_PRINT("Failed to create camera component");
			vcos_log_error("Failed to create camera component");
	      	break;
	   }

	 	DBG_PRINT("--- raspicamcontrol_set_stereo_mode ---");
	 	
	   int temp;
	   temp = raspicamcontrol_set_stereo_mode(camera->output[0], &state->camera_parameters.stereo_mode);
	   temp += raspicamcontrol_set_stereo_mode(camera->output[1], &state->camera_parameters.stereo_mode);
	   temp += raspicamcontrol_set_stereo_mode(camera->output[2], &state->camera_parameters.stereo_mode);
	   status = (MMAL_STATUS_T)temp;
	   if (status != MMAL_SUCCESS)
	   {
			DBG_PRINT("Could not set stereo mode : error %d", status);
	      vcos_log_error("Could not set stereo mode : error %d", status);
	      break;
	   }

	 	DBG_PRINT("--- mmal_port_parameter_set ---");

	   MMAL_PARAMETER_INT32_T camera_num =
	      {{MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)}, state->cameraNum};

	   status = mmal_port_parameter_set(camera->control, &camera_num.hdr);

	   if (status != MMAL_SUCCESS)
	   {
			DBG_PRINT("Could not select camera : error %d", status);
	      vcos_log_error("Could not select camera : error %d", status);
	      break;
	   }

	   if (!camera->output_num)
	   {
	      status = MMAL_ENOSYS;
			DBG_PRINT("Camera doesn't have output ports");
	      vcos_log_error("Camera doesn't have output ports");
	      break;
	   }

	 	DBG_PRINT("--- mmal_port_parameter_set_uint32 ---");

	   status = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG, state->sensor_mode);

	   if (status != MMAL_SUCCESS)
	   {
			DBG_PRINT("Could not set sensor mode : error %d", status);
	      vcos_log_error("Could not set sensor mode : error %d", status);
	      break;
	   }

	   preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
	   video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
	   still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

	   if (state->settings)
	   {
	      MMAL_PARAMETER_CHANGE_EVENT_REQUEST_T change_event_request =
	         {{MMAL_PARAMETER_CHANGE_EVENT_REQUEST, sizeof(MMAL_PARAMETER_CHANGE_EVENT_REQUEST_T)},
	          MMAL_PARAMETER_CAMERA_SETTINGS, 1};

	      status = mmal_port_parameter_set(camera->control, &change_event_request.hdr);
	      if ( status != MMAL_SUCCESS )
	      {
			DBG_PRINT("No camera settings events");
	         vcos_log_error("No camera settings events");
	      }
	   }

	 	DBG_PRINT("--- mmal_port_enable ---");

	   // Enable the camera, and tell it its control callback function
	   status = mmal_port_enable(camera->control, camera_control_callback);

	   if (status != MMAL_SUCCESS)
	   {
			DBG_PRINT("Unable to enable control port : error %d", status);
	      vcos_log_error("Unable to enable control port : error %d", status);
	      break;
	   }

	   //  set up the camera configuration
	   {
	      MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
	      {
	         { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
	         .max_stills_w = static_cast<uint32_t>(state->width),
	         .max_stills_h = static_cast<uint32_t>(state->height),
	         .stills_yuv422 = 0,
	         .one_shot_stills = 0,
	         .max_preview_video_w = static_cast<uint32_t>(state->width),
	         .max_preview_video_h = static_cast<uint32_t>(state->height),
	         .num_preview_video_frames = static_cast<uint32_t>(3 + vcos_max(0, (state->framerate-30)/10)),
	         .stills_capture_circular_buffer_height = 0,
	         .fast_preview_resume = 0,
	         .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RAW_STC
	      };
	      mmal_port_parameter_set(camera->control, &cam_config.hdr);
	   }

	   // Now set up the port formats

	   // Set the encode format on the Preview port
	   // HW limitations mean we need the preview to be the same size as the required recorded output

	   format = preview_port->format;

	   format->encoding = MMAL_ENCODING_OPAQUE;
	   format->encoding_variant = MMAL_ENCODING_I420;

	   if(state->camera_parameters.shutter_speed > 6000000)
	   {
	        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
	                                                     { 50, 1000 }, {166, 1000}};
	        mmal_port_parameter_set(preview_port, &fps_range.hdr);
	   }
	   else if(state->camera_parameters.shutter_speed > 1000000)
	   {
	        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
	                                                     { 166, 1000 }, {999, 1000}};
	        mmal_port_parameter_set(preview_port, &fps_range.hdr);
	   }

	   //enable dynamic framerate if necessary
	   if (state->camera_parameters.shutter_speed)
	   {
	      if (state->framerate > 1000000./state->camera_parameters.shutter_speed)
	      {
	         state->framerate=0;
			DBG_PRINT("Enable dynamic frame rate to fulfil shutter speed requirement");
	         DBG_PRINT("Enable dynamic frame rate to fulfil shutter speed requirement");
	      }
	   }

	   format->encoding                 = MMAL_ENCODING_OPAQUE;
	   format->es->video.width          = VCOS_ALIGN_UP(state->width, 32);
	   format->es->video.height         = VCOS_ALIGN_UP(state->height, 16);
	   format->es->video.crop.x         = 0;
	   format->es->video.crop.y         = 0;
	   format->es->video.crop.width     = state->width;
	   format->es->video.crop.height    = state->height;
	   format->es->video.frame_rate.num = 0;
	   format->es->video.frame_rate.den = 1;

	   status = mmal_port_format_commit(preview_port);
	   if (status != MMAL_SUCCESS)
	   {
	      DBG_PRINT("camera viewfinder format couldn't be set");
	      // goto error;
	      break;
	   }
   
	   // Set the encode format on the video  port

	   format = video_port->format;
	   format->encoding_variant = MMAL_ENCODING_I420;

	   if(state->camera_parameters.shutter_speed > 6000000)
	   {
	        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
	                                                     { 50, 1000 }, {166, 1000}};
	        mmal_port_parameter_set(video_port, &fps_range.hdr);
	   }
	   else if(state->camera_parameters.shutter_speed > 1000000)
	   {
	        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
	                                                     { 167, 1000 }, {999, 1000}};
	        mmal_port_parameter_set(video_port, &fps_range.hdr);
	   }

	   format->encoding = MMAL_ENCODING_OPAQUE;
	   format->es->video.width = VCOS_ALIGN_UP(state->width, 32);
	   format->es->video.height = VCOS_ALIGN_UP(state->height, 16);
	   format->es->video.crop.x = 0;
	   format->es->video.crop.y = 0;
	   format->es->video.crop.width = state->width;
	   format->es->video.crop.height = state->height;
	   format->es->video.frame_rate.num = state->framerate;
	   format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;

	   status = mmal_port_format_commit(video_port);

	   if (status != MMAL_SUCCESS)
	   {
			DBG_PRINT("camera video format couldn't be set");
	      vcos_log_error("camera video format couldn't be set");
	      break;
	   }

	   // Ensure there are enough buffers to avoid dropping frames
	   if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
	      video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

		status = mmal_port_parameter_set_boolean(video_port, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
   		if (status != MMAL_SUCCESS)
   		{
			DBG_PRINT("camera video format couldn't be set");
			vcos_log_error("Failed to select zero copy");
			break;
		}

	   // Set the encode format on the still  port

	   format = still_port->format;

	   format->encoding = MMAL_ENCODING_OPAQUE;
	   format->encoding_variant = MMAL_ENCODING_I420;

	   format->es->video.width          = VCOS_ALIGN_UP(state->width, 32);
	   format->es->video.height         = VCOS_ALIGN_UP(state->height, 16);
	   format->es->video.crop.x         = 0;
	   format->es->video.crop.y         = 0;
	   format->es->video.crop.width     = state->width;
	   format->es->video.crop.height    = state->height;
	   format->es->video.frame_rate.num = 0;
	   format->es->video.frame_rate.den = 1;

	   status = mmal_port_format_commit(still_port);

	   if (status != MMAL_SUCCESS)
	   {
			DBG_PRINT("camera still format couldn't be set");
	      vcos_log_error("camera still format couldn't be set");
	      break;
	   }

	   /* Ensure there are enough buffers to avoid dropping frames */
	   if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
	      still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

	   /* Enable component */
		DBG_PRINT("--- mmal_component_enable ---");
		status = mmal_component_enable(camera);
		if (status != MMAL_SUCCESS)
		{
			DBG_PRINT("camera component couldn't be enabled");
			vcos_log_error("camera component couldn't be enabled");
			break;
		}

		raspicamcontrol_set_all_parameters(camera, &state->camera_parameters);

		camera_component = camera;

		memset(m_annotation_text, 0x00, sizeof(m_annotation_text));
		update_annotation_data();

		DBG_TRACE("ExitRaspiCaptureDevice::create_camera_component");

		return status;
	}
	while(0);

	if (camera)
		mmal_component_destroy(camera);

	return status;
}

void RaspiCaptureDevice::update_annotation_data()
{
	SYSTEMTIME system_time;
	GetLocalTime(&system_time);

	char text[80];
	sprintf(
		text,
		"%4d/%02d/%02d %02d:%02d:%02d",
		(int)system_time.wYear,
		(int)system_time.wMonth,
		(int)system_time.wDay,
		(int)system_time.wHour,
		(int)system_time.wMinute,
		(int)system_time.wSecond
		);
	
	if (strcmp(m_annotation_text, text) != 0)
	{
		raspicamcontrol_set_annotate(
			camera_component, 
			ANNOTATE_USER_TEXT,
			text,
			0,
			-1,
			-1
	    );
	    strcpy(m_annotation_text, text);
	}
}

MMAL_STATUS_T RaspiCaptureDevice::create_splitter_component(RASPIVID_STATE *state)
{
   MMAL_COMPONENT_T *splitter = 0;
   MMAL_PORT_T *splitter_output = NULL;
   MMAL_ES_FORMAT_T *format;
   MMAL_STATUS_T status;
   MMAL_POOL_T *pool;
   int i;

   /* Create the component */
   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_SPLITTER, &splitter);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Failed to create splitter component");
      goto error;
   }

   if (!splitter->input_num)
   {
      status = MMAL_ENOSYS;
      vcos_log_error("Splitter doesn't have any input port");
      goto error;
   }

   if (splitter->output_num < 2)
   {
      status = MMAL_ENOSYS;
      vcos_log_error("Splitter doesn't have enough output ports");
      goto error;
   }

   /* Ensure there are enough buffers to avoid dropping frames: */
   mmal_format_copy(splitter->input[0]->format, camera_component->output[MMAL_CAMERA_PREVIEW_PORT]->format);

   if (splitter->input[0]->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      splitter->input[0]->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

   status = mmal_port_format_commit(splitter->input[0]);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to set format on splitter input port");
      goto error;
   }

   /* Splitter can do format conversions, configure format for its output port: */
   for (i = 0; i < splitter->output_num; i++)
   {
      mmal_format_copy(splitter->output[i]->format, splitter->input[0]->format);

      if (i == SPLITTER_OUTPUT_PORT)
      {
         format = splitter->output[i]->format;
	     format->encoding = MMAL_ENCODING_I420;
         format->encoding_variant = MMAL_ENCODING_I420;
      }

      status = mmal_port_format_commit(splitter->output[i]);

      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set format on splitter output port %d", i);
         goto error;
      }
   }

   /* Enable component */
   status = mmal_component_enable(splitter);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("splitter component couldn't be enabled");
      goto error;
   }

   /* Create pool of buffer headers for the output port to consume */
   splitter_output = splitter->output[SPLITTER_OUTPUT_PORT];
   pool = mmal_port_pool_create(splitter_output, splitter_output->buffer_num, splitter_output->buffer_size);

   if (!pool)
   {
      vcos_log_error("Failed to create buffer header pool for splitter output port %s", splitter_output->name);
   }

   splitter_pool = pool;
   splitter_component = splitter;

   return status;

error:

   if (splitter)
      mmal_component_destroy(splitter);

   return status;
}

MMAL_STATUS_T RaspiCaptureDevice::create_encoder_component(RASPIVID_STATE *state)
{
	DBG_TRACE("Enter RaspiCaptureDevice::create_encoder_component");
	
   MMAL_COMPONENT_T *encoder = 0;
   MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL;
   MMAL_STATUS_T status;
   MMAL_POOL_T *pool;

   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER, &encoder);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to create video encoder component");
      goto error;
   }

   if (!encoder->input_num || !encoder->output_num)
   {
      status = MMAL_ENOSYS;
      vcos_log_error("Video encoder doesn't have input/output ports");
      goto error;
   }

   encoder_input  = encoder->input[0];
   encoder_output = encoder->output[0];

   // We want same format on input and output
   mmal_format_copy(encoder_output->format, encoder_input->format);

   // Only supporting H264 at the moment
   encoder_output->format->encoding = state->encoding;

   if(state->encoding == MMAL_ENCODING_H264)
   {
      if(state->level == MMAL_VIDEO_LEVEL_H264_4)
      {
         if(state->bitrate > MAX_BITRATE_LEVEL4)
         {
            fprintf(stderr, "Bitrate too high: Reducing to 25MBit/s\n");
            state->bitrate = MAX_BITRATE_LEVEL4;
         }
      }
      else
      {
         if(state->bitrate > MAX_BITRATE_LEVEL42)
         {
            fprintf(stderr, "Bitrate too high: Reducing to 62.5MBit/s\n");
            state->bitrate = MAX_BITRATE_LEVEL42;
         }
      }
   }
   else if(state->encoding == MMAL_ENCODING_MJPEG)
   {
      if(state->bitrate > MAX_BITRATE_MJPEG)
      {
         fprintf(stderr, "Bitrate too high: Reducing to 25MBit/s\n");
         state->bitrate = MAX_BITRATE_MJPEG;
      }
   }
   
   encoder_output->format->bitrate = state->bitrate;

	DBG_PRINT("buffer_size_recommended = %d", encoder_output->buffer_size_recommended);

	if (state->encoding == MMAL_ENCODING_H264)
	{
      	encoder_output->buffer_size = encoder_output->buffer_size_recommended * 4;		// #TIPS# recommended ‚¾‚Æ1920x1080‚Å‘«‚ç‚È‚¢
	}
	else
	{
    	encoder_output->buffer_size = 256<<10;
	}

	if (encoder_output->buffer_size < encoder_output->buffer_size_min)
   	{
    	encoder_output->buffer_size = encoder_output->buffer_size_min;
	}
	DBG_PRINT("buffer_size = %d", encoder_output->buffer_size);
  
	DBG_PRINT("buffer_num_recommended = %d", encoder_output->buffer_num_recommended);
	// encoder_output->buffer_num = encoder_output->buffer_num_recommended;
	encoder_output->buffer_num = 4;
	DBG_PRINT("buffer_num = %d", encoder_output->buffer_num);

   if (encoder_output->buffer_num < encoder_output->buffer_num_min)
      encoder_output->buffer_num = encoder_output->buffer_num_min;

   // We need to set the frame rate on output to 0, to ensure it gets
   // updated correctly from the input framerate when port connected
   encoder_output->format->es->video.frame_rate.num = 0;
   encoder_output->format->es->video.frame_rate.den = 1;

   // Commit the port changes to the output port
   status = mmal_port_format_commit(encoder_output);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to set format on video encoder output port");
      goto error;
   }

   // Set the rate control parameter
   if (0)
   {
      MMAL_PARAMETER_VIDEO_RATECONTROL_T param = {{ MMAL_PARAMETER_RATECONTROL, sizeof(param)}, MMAL_VIDEO_RATECONTROL_DEFAULT};
      status = mmal_port_parameter_set(encoder_output, &param.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set ratecontrol");
         goto error;
      }

   }

   if (state->encoding == MMAL_ENCODING_H264 &&
       state->intraperiod != -1)
   {
      MMAL_PARAMETER_UINT32_T param = {{ MMAL_PARAMETER_INTRAPERIOD, sizeof(param)}, static_cast<uint32_t>(state->intraperiod)};
      status = mmal_port_parameter_set(encoder_output, &param.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set intraperiod");
         goto error;
      }
   }

   if (state->encoding == MMAL_ENCODING_H264 &&
       state->quantisationParameter)
   {
      MMAL_PARAMETER_UINT32_T param = {{ MMAL_PARAMETER_VIDEO_ENCODE_INITIAL_QUANT, sizeof(param)}, static_cast<uint32_t>(state->quantisationParameter)};
      status = mmal_port_parameter_set(encoder_output, &param.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set initial QP");
         goto error;
      }

      MMAL_PARAMETER_UINT32_T param2 = {{ MMAL_PARAMETER_VIDEO_ENCODE_MIN_QUANT, sizeof(param)}, static_cast<uint32_t>(state->quantisationParameter)};
      status = mmal_port_parameter_set(encoder_output, &param2.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set min QP");
         goto error;
      }

      MMAL_PARAMETER_UINT32_T param3 = {{ MMAL_PARAMETER_VIDEO_ENCODE_MAX_QUANT, sizeof(param)}, static_cast<uint32_t>(state->quantisationParameter)};
      status = mmal_port_parameter_set(encoder_output, &param3.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set max QP");
         goto error;
      }

   }

   if (state->encoding == MMAL_ENCODING_H264)
   {
      MMAL_PARAMETER_VIDEO_PROFILE_T  param;
      param.hdr.id = MMAL_PARAMETER_PROFILE;
      param.hdr.size = sizeof(param);

      param.profile[0].profile = (MMAL_VIDEO_PROFILE_T)state->profile;

      if((VCOS_ALIGN_UP(state->width,16) >> 4) * (VCOS_ALIGN_UP(state->height,16) >> 4) * state->framerate > 245760)
      {
         if((VCOS_ALIGN_UP(state->width,16) >> 4) * (VCOS_ALIGN_UP(state->height,16) >> 4) * state->framerate <= 522240)
         {
            fprintf(stderr, "Too many macroblocks/s: Increasing H264 Level to 4.2\n");
            state->level=MMAL_VIDEO_LEVEL_H264_42;
         }
         else
         {
            vcos_log_error("Too many macroblocks/s requested");
            goto error;
         }
      }
      
      param.profile[0].level = (MMAL_VIDEO_LEVEL_T)state->level;

      status = mmal_port_parameter_set(encoder_output, &param.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set H264 profile");
         goto error;
      }
   }

   if (mmal_port_parameter_set_boolean(encoder_input, MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT, 0) != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to set immutable input flag");
      // Continue rather than abort..
   }

   //set INLINE HEADER flag to generate SPS and PPS for every IDR if requested
   if (mmal_port_parameter_set_boolean(encoder_output, MMAL_PARAMETER_VIDEO_ENCODE_INLINE_HEADER, 0) != MMAL_SUCCESS)
   {
      vcos_log_error("failed to set INLINE HEADER FLAG parameters");
      // Continue rather than abort..
   }

   // Adaptive intra refresh settings
   if (state->encoding == MMAL_ENCODING_H264 &&
       state->intra_refresh_type != -1)
   {
      MMAL_PARAMETER_VIDEO_INTRA_REFRESH_T  param;
      param.hdr.id = MMAL_PARAMETER_VIDEO_INTRA_REFRESH;
      param.hdr.size = sizeof(param);

      // Get first so we don't overwrite anything unexpectedly
      status = mmal_port_parameter_get(encoder_output, &param.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_warn("Unable to get existing H264 intra-refresh values. Please update your firmware");
         // Set some defaults, don't just pass random stack data
         param.air_mbs = param.air_ref = param.cir_mbs = param.pir_mbs = 0;
      }

      param.refresh_mode = (MMAL_VIDEO_INTRA_REFRESH_T)(state->intra_refresh_type);

      //if (state->intra_refresh_type == MMAL_VIDEO_INTRA_REFRESH_CYCLIC_MROWS)
      //   param.cir_mbs = 10;

      status = mmal_port_parameter_set(encoder_output, &param.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set H264 intra-refresh values");
         goto error;
      }
   }

   //  Enable component
   status = mmal_component_enable(encoder);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to enable video encoder component");
      goto error;
   }

   /* Create pool of buffer headers for the output port to consume */
   pool = mmal_port_pool_create(encoder_output, encoder_output->buffer_num, encoder_output->buffer_size);

   if (!pool)
   {
      vcos_log_error("Failed to create buffer header pool for encoder output port %s", encoder_output->name);
   }

   encoder_pool = pool;
   encoder_component = encoder;

   return status;

   error:
   if (encoder)
      mmal_component_destroy(encoder);

   encoder_component = NULL;

   return status;
}

void RaspiCaptureDevice::check_disable_port(MMAL_PORT_T *port)
{
   if (port && port->is_enabled)
      mmal_port_disable(port);
}

/**
 * Destroy the camera component
 *
 * @param state Pointer to state control struct
 *
 */
void RaspiCaptureDevice::destroy_camera_component()
{
	DBG_TRACE("Enter destroy_camera_component");
	
	if (camera_component)
	{
		mmal_component_destroy(camera_component);
		camera_component = NULL;
	}
	
	DBG_TRACE("Exit destroy_camera_component");
}

/**
 * Destroy the splitter component
 *
 * @param state Pointer to state control struct
 *
 */
void RaspiCaptureDevice::destroy_splitter_component()
{
   // Get rid of any port buffers first
   if (splitter_pool)
   {
      mmal_port_pool_destroy(splitter_component->output[SPLITTER_OUTPUT_PORT], splitter_pool);
      splitter_pool = NULL;
   }

   if (splitter_component)
   {
      mmal_component_destroy(splitter_component);
      splitter_component = NULL;
   }
}

/**
 * Destroy the encoder component
 *
 * @param state Pointer to state control struct
 *
 */
void RaspiCaptureDevice::destroy_encoder_component()
{
   // Get rid of any port buffers first
   if (encoder_pool)
   {
      mmal_port_pool_destroy(encoder_component->output[0], encoder_pool);
      encoder_pool = NULL;
   }

   if (encoder_component)
   {
      mmal_component_destroy(encoder_component);
      encoder_component = NULL;
   }
}

/**
 * Connect two specific ports together
 *
 * @param output_port Pointer the output port
 * @param input_port Pointer the input port
 * @param Pointer to a mmal connection pointer, reassigned if function successful
 * @return Returns a MMAL_STATUS_T giving result of operation
 *
 */
MMAL_STATUS_T RaspiCaptureDevice::connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection)
{
   MMAL_STATUS_T status;

	status =  mmal_connection_create(connection, output_port, input_port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
	if (status != MMAL_SUCCESS)
	{
		return status;
	}
	
	status = mmal_connection_enable(*connection);
	if (status != MMAL_SUCCESS)
	{
		mmal_connection_destroy(*connection);
	}

	return status;
}

bool RaspiCaptureDevice::OnStart(CaptureParam* param)
{
	DBG_TRACE("Enter RaspiCaptureDevice::OnStart");
	
	RASPIVID_STATE state;

	// Default everything to zero
	memset(&state, 0, sizeof(RASPIVID_STATE));

	// Now set anything non-zero
	state.width                 = 1280;
	state.height                = 720;
	state.encoding              = MMAL_ENCODING_H264;
	state.bitrate               = 2000000;
	state.framerate             = VIDEO_FRAME_RATE_NUM;
	state.intraperiod           = -1;    // Not set
	state.quantisationParameter = 0;
	state.profile               = MMAL_VIDEO_PROFILE_H264_HIGH;
	state.level                 = MMAL_VIDEO_LEVEL_H264_4;

	state.cameraNum = 0;
	state.settings = 0;
	state.sensor_mode = 0;

	state.intra_refresh_type = -1;

	// Set up the camera_parameters to default
	raspicamcontrol_set_defaults(&state.camera_parameters);
	
	state.width     = param->width;
	state.height    = param->height;
	state.framerate = param->framerate;
	state.bitrate   = param->bitrate;
	
	// 1•b–ˆ‚ÉKeyFrame‚ÌoŒ»‚³‚¹‚é
	state.intraperiod = state.framerate;
	
	DBG_PRINT("width  = %d", state.width);
	DBG_PRINT("height = %d", state.height);
	
	DBG_PRINT("--- vcos_log_register ---");
	vcos_log_register("RaspiCaptureDevice", VCOS_LOG_CATEGORY);
	
	DBG_PRINT("--- create_camera_component ---");
	if (create_camera_component(&state) != MMAL_SUCCESS)
	{
		DBG_PRINT("Error: create_camera_component");
		Terminate();
		return false;
   	}
	DBG_PRINT("--- create_encoder_component ---");
 	if (create_encoder_component(&state) != MMAL_SUCCESS)
   	{
		DBG_PRINT("Error: create_encoder_component");
		Terminate();
      	return false;
	}
	DBG_PRINT("--- create_splitter_component ---");
	if (create_splitter_component(&state) != MMAL_SUCCESS)
   	{
		DBG_PRINT("Error: create_splitter_component");
		Terminate();
      	return false;
	}
	
	camera_preview_port  = camera_component->output[MMAL_CAMERA_PREVIEW_PORT];
	camera_video_port    = camera_component->output[MMAL_CAMERA_VIDEO_PORT];
	splitter_input_port  = splitter_component->input[0];
	splitter_output_port = splitter_component->output[SPLITTER_OUTPUT_PORT];
	encoder_input_port   = encoder_component->input[0];
	encoder_output_port  = encoder_component->output[0];

	DBG_PRINT("connect_ports(camera_preview_port)");
	
	if (connect_ports(camera_preview_port, splitter_input_port, &splitter_connection) != MMAL_SUCCESS)
	{
		DBG_PRINT("Error: connect_ports(splitter)");
		Terminate();
		return false;
	}
	
	DBG_PRINT("connect_ports(camera_video_port)");
	
	// Now connect the camera to the encoder
	if (connect_ports(camera_video_port, encoder_input_port, &encoder_connection) != MMAL_SUCCESS)
	{
		DBG_PRINT("Error: connect_ports(video)");
		Terminate();
		return false;
	}

	sps_size = 0;
	pps_size = 0;
	m_max_packet_size = 0;
	
	DBG_PRINT("RaspiCaptureDevice PoolSize = %d", encoder_output_port->buffer_size);
	
	m_packet_pool = new MH_MediaPacketPool("RaspiCaptureDevice");
	m_packet_pool->Create(
		encoder_output_port->buffer_size,
		state.framerate,
		false
	);
	
	m_snapshot_event = new MH_Event();
	m_snapshot_event->Create();
	
	MMAL_STATUS_T status = MMAL_SUCCESS;
   
	m_first_time = -1;
	m_first_pts  = -1;
   
    splitter_output_port->userdata = (MMAL_PORT_USERDATA_T *)this;

	DBG_PRINT("mmal_port_enable(splitter_output_port)");

    // Enable the splitter output port and tell it its callback function
    status = mmal_port_enable(splitter_output_port, splitter_buffer_callback);
    if (status != MMAL_SUCCESS)
    {
    	return false;
    }
   
	// Set up our userdata - this is passed though to the callback where we need the information.
	encoder_output_port->userdata = (MMAL_PORT_USERDATA_T*)this;

	DBG_PRINT("mmal_port_enable(encoder_output_port)");
	
	// Enable the encoder output port and tell it its callback function
	status = mmal_port_enable(encoder_output_port, encoder_buffer_callback);
	if (status != MMAL_SUCCESS)
	{
		return false;
	}

	// Send all the buffers to the encoder output port
	int num = mmal_queue_length(encoder_pool->queue);
    for (int q = 0; q < num; q++)
    {
    	MMAL_BUFFER_HEADER_T* buffer = mmal_queue_get(encoder_pool->queue);
		if (!buffer)
		{
        	vcos_log_error("Unable to get a required buffer %d from pool queue", q);
        	break;
        }

		if (mmal_port_send_buffer(encoder_output_port, buffer)!= MMAL_SUCCESS)
		{
			vcos_log_error("Unable to send a buffer to encoder output port (%d)", q);
        	break;
		}
	}

	if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
    {
         return false;
    }

	DBG_TRACE("Exit RaspiCaptureDevice::OnStart");

    return true;
}

void RaspiCaptureDevice::OnStop()
{
	DBG_TRACE("Enter RaspiCaptureDevice::OnStop");
	
	if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 0) != MMAL_SUCCESS)
    {
         return;
    }

	check_disable_port(encoder_output_port);
	check_disable_port(splitter_output_port);

	if (encoder_connection)
	{
		mmal_connection_disable(encoder_connection);
		mmal_connection_destroy(encoder_connection);
		encoder_connection = NULL;
	}
	if (splitter_connection)
	{
		mmal_connection_disable(splitter_connection);
		mmal_connection_destroy(splitter_connection);
		splitter_connection = NULL;
	}

	if (encoder_component)
	{
		mmal_component_disable(encoder_component);
	}
	if (splitter_component)
	{
		mmal_component_disable(splitter_component);
	}
	if (camera_component)
	{
		mmal_component_disable(camera_component);
	}

	destroy_encoder_component();
	destroy_splitter_component();
    destroy_camera_component();
    
	if (m_snapshot_event != NULL)
	{
		m_snapshot_event->Close();
		delete m_snapshot_event;
		m_snapshot_event = NULL;
	}
	
	if (m_packet_pool != NULL)
	{
		m_packet_pool->Close();
		m_packet_pool = NULL;
	}
	
	DBG_TRACE("Exit RaspiCaptureDevice::OnStop");
}

void RaspiCaptureDevice::camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   if (buffer->cmd == MMAL_EVENT_PARAMETER_CHANGED)
   {
      MMAL_EVENT_PARAMETER_CHANGED_T *param = (MMAL_EVENT_PARAMETER_CHANGED_T *)buffer->data;
      switch (param->hdr.id) {
         case MMAL_PARAMETER_CAMERA_SETTINGS:
         {
            MMAL_PARAMETER_CAMERA_SETTINGS_T *settings = (MMAL_PARAMETER_CAMERA_SETTINGS_T*)param;
            vcos_log_error("Exposure now %u, analog gain %u/%u, digital gain %u/%u",
			settings->exposure,
                        settings->analog_gain.num, settings->analog_gain.den,
                        settings->digital_gain.num, settings->digital_gain.den);
            vcos_log_error("AWB R=%u/%u, B=%u/%u",
                        settings->awb_red_gain.num, settings->awb_red_gain.den,
                        settings->awb_blue_gain.num, settings->awb_blue_gain.den
                        );
         }
         break;
      }
   }
   else if (buffer->cmd == MMAL_EVENT_ERROR)
   {
      vcos_log_error("No data received from sensor. Check all connections, including the Sunny one on the camera board");
   }
   else
   {
      vcos_log_error("Received unexpected camera control callback event, 0x%08x", buffer->cmd);
   }

   mmal_buffer_header_release(buffer);
}

void RaspiCaptureDevice::encoder_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	RaspiCaptureDevice* own = (RaspiCaptureDevice*)(port->userdata);
	own->EnoderBufferCallback(port, buffer);
}

void RaspiCaptureDevice::EnoderBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	if (buffer->length)
	{
		mmal_buffer_header_mem_lock(buffer);
		
	    if(buffer->flags & MMAL_BUFFER_HEADER_FLAG_CODECSIDEINFO)
	    {
	    }
	    else if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_CONFIG )
	    {
	    	//--------------------
	    	// sps, pps
	    	//--------------------

			unsigned char* data = buffer->data;
			int pos = 0;
			for(pos = 4; pos < buffer->length - 4; pos++)
			{
				if (data[pos] == 0x00 && data[pos + 1] == 0x00 && data[pos + 2] == 0x00 && data[pos + 3] == 0x01)
				{
					sps_size = pos;
					memcpy(sps, &data[0], sps_size);

					pps_size = buffer->length - pos;
					memcpy(pps, &data[pos], pps_size);

					DBG_PRINT("+++ sps : size = %d +++", sps_size);
					DBG_DUMP(sps, sps_size);
					DBG_PRINT("+++ pps : size = %d +++", pps_size);
					DBG_DUMP(pps, pps_size);
					break;
				}
			}
	    }
	    else
	    {
	    	if (m_first_time == -1)
	    	{
			  	struct timeval timeNow;
			  	gettimeofday(&timeNow, NULL);
			  	
				m_first_time = (int64_t)(timeNow.tv_sec) * 1000000LL + (int64_t)(timeNow.tv_usec);
				m_first_pts  = buffer->pts;
			}
			
			int64_t pts = m_first_time + (buffer->pts - m_first_pts);
				
	    	MH_MediaPacket* media_packet = NULL;
	    	
	    	if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_KEYFRAME)
	    	{
				if (4 < sps_size)
				{
					media_packet = m_packet_pool->GetMediaPacket(sps_size - 4, 0);
					if (media_packet != NULL)
					{
						memcpy(media_packet->m_data, &sps[4], sps_size - 4);
						media_packet->m_size      = sps_size - 4;
						media_packet->m_type      = TYPE_VIDEO_DATA;
						media_packet->m_pts       = pts;
						media_packet->m_dts       = pts;
						media_packet->m_duration  = 0;
				    	media_packet->m_key_frame = true;
						Push(media_packet);
						media_packet->ReleaseRef();
					}
					else
					{
						DBG_PRINT("Warnning: Not Engouth Packet Pool for sps");
					}
				}

				if (4 < pps_size)
				{
					media_packet = m_packet_pool->GetMediaPacket(pps_size - 4, 0);
					if (media_packet != NULL)
					{
						memcpy(media_packet->m_data, &pps[4], pps_size - 4);
						media_packet->m_size      = pps_size - 4;
						media_packet->m_type      = TYPE_VIDEO_DATA;
						media_packet->m_pts       = pts;
						media_packet->m_dts       = pts;
						media_packet->m_duration  = 0;
						media_packet->m_key_frame = false;
						Push(media_packet);
						media_packet->ReleaseRef();
					}
					else
					{
						DBG_PRINT("Warnning: Not Engouth Packet Pool for pps");
					}
				}
	    	}	
	    	
	    	int packet_size = buffer->length - 4;
	    	if (m_max_packet_size < packet_size)
	    	{
	    		m_max_packet_size = packet_size;
	    		DBG_PRINT("m_max_packet_size = %d", m_max_packet_size);
	    	}
			media_packet = m_packet_pool->GetMediaPacket(buffer->length - 4, 0);
	    	if (media_packet != NULL)
	    	{
	    		if (media_packet->m_buffer_size < buffer->length - 4)
	    		{
					DBG_PRINT("Warnning: buffer size is small (%d < %d)", media_packet->m_buffer_size, buffer->length - 4);
	    		}
	    		else
	    		{
			    	memcpy(media_packet->m_data, buffer->data + 4, buffer->length - 4);
			    	media_packet->m_size      = buffer->length;
			    	media_packet->m_key_frame = 0;
		    		media_packet->m_type      = TYPE_VIDEO_DATA;
					media_packet->m_pts       = pts;
					media_packet->m_dts       = pts;
			    	media_packet->m_duration  = 0;
			    	media_packet->m_key_frame = false;
			    	Push(media_packet);
			    }
			 	media_packet->ReleaseRef();
		    }
		    else
		    {
		    	DBG_PRINT("Warnning: Not Engouth Packet Pool");
		    }
	    }

	    mmal_buffer_header_mem_unlock(buffer);
   }

	update_annotation_data();
	
   // release buffer back to the pool
   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled)
   {
		MMAL_STATUS_T status = MMAL_ENOMEM;
		
		MMAL_BUFFER_HEADER_T* new_buffer = mmal_queue_get(encoder_pool->queue);
		if (new_buffer)
		{
			status = mmal_port_send_buffer(port, new_buffer);
		}

		if (status != MMAL_SUCCESS)
		{
			vcos_log_error("Unable to return a buffer to the encoder port");
		}
   }
}

extern "C" int mmal_status_to_int(MMAL_STATUS_T status);

bool RaspiCaptureDevice::OnSnapShot(MH_MediaPacket* snapshot)
{
	// DBG_TRACE("Enter RaspiCaptureDevice::SnapShot");
	
	m_snapshot = snapshot;
	
    MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(splitter_pool->queue);
    mmal_port_send_buffer(splitter_output_port, buffer);
	
	m_snapshot_event->Wait();
	
	// DBG_TRACE("Exit RaspiCaptureDevice::SnapShot");
	
	return true;
}

void RaspiCaptureDevice::splitter_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	RaspiCaptureDevice* own = (RaspiCaptureDevice*)(port->userdata);
	own->SplitterBufferCallback(port, buffer);
}

void RaspiCaptureDevice::SplitterBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	// DBG_TRACE("Enter RaspiCaptureDevice::SplitterBufferCallback");
	// DBG_PRINT("port->is_enabled = %d", port->is_enabled);
	// DBG_PRINT("buffer->length   = %d", buffer->length);

	int alignW    = VCOS_ALIGN_UP(m_param.width,  32);
	int alignH    = VCOS_ALIGN_UP(m_param.height, 16);
	int alignY    = alignW     * alignH;
	int alignUV   = alignW / 2 * alignH / 2;
	int alignSize = alignY + alignUV * 2;

	int sizeY   = m_param.width     * m_param.height;
	int sizeUV  = m_param.width / 2 * m_param.height / 2;
	int size    = sizeY + sizeUV * 2;

    if (alignSize == buffer->length && size == m_snapshot->m_buffer_size)
    {
		mmal_buffer_header_mem_lock(buffer);
		
		memcpy(m_snapshot->m_buffer,                  buffer->data,                    sizeY);
		memcpy(m_snapshot->m_buffer + sizeY,          buffer->data + alignY,           sizeUV);
		memcpy(m_snapshot->m_buffer + sizeY + sizeUV, buffer->data + alignY + alignUV, sizeUV);

		m_snapshot->m_size = sizeY + sizeUV * 2;
		
		mmal_buffer_header_mem_unlock(buffer);
	}
	else
	{
		DBG_PRINT("Error: Invalid Buffer Size : %d, %d, %d, %d",
			alignSize,
			buffer->length,
			size,
			m_snapshot->m_buffer_size
		);
	}
	
	mmal_buffer_header_release(buffer);
	
	m_snapshot_event->Set();
	
	// DBG_TRACE("Exit RaspiCaptureDevice::SplitterBufferCallback");
}
