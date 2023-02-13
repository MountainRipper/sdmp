#ifndef CORE_INCLUDES_H
#define CORE_INCLUDES_H

#include "sdp_general_filter_base.h"
#include "sdpi_graph.h"
#include "sdpi_factory.h"
#include "sdpi_utils.h"
#include <logger.h>

extern "C"
{

#include <stdint.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/common.h>
#include <libavutil/intreadwrite.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/jni.h>

#include <libavformat/avformat.h>

#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <libavdevice/avdevice.h>
}


#endif // CORE_INCLUDES_H
