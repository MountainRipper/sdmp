#ifndef SDP_OBJECTS_H
#define SDP_OBJECTS_H

#include <sdpi_filter.h>
#include <sdpi_filter_extentions.h>

#include "../core_filters/internal/audio_output_device_miniaudio.h"
#include "../core_filters/media_source_ffmpeg.h"
#include "../core_filters/media_source_memory.h"
#include "../core_filters/video_capture_ffmpeg.h"

#include "../core_filters/media_cache_filter.h"
#include "../core_filters/audio_decoder_ffmpeg.h"
#include "../core_filters/audio_encoder_ffmpeg.h"
#include "../core_filters/video_decoder_ffmpeg.h"
#include "../core_filters/video_decoder_rkmpp.h"
#include "../core_filters/video_encoder_ffmpeg.h"
#include "../core_filters/data_grabber.h"

#include "../core_filters/video_output_proxy.h"
#include "../core_filters/audio_output_participant.h"
#include "../core_filters/media_muxer_ffmpeg.h"

#include "../core_stubs/video_render_stub_opengl.h"
#include "../core_stubs/video_render_stub_kms.h"

#endif // SDP_OBJECTS_H
