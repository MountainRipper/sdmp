#ifndef CORE_INCLUDES_H
#define CORE_INCLUDES_H

#include <mr/logger.h>
#include <tio/tio_software_frame.h>
#include "sdmpi_graph.h"
#include "sdmpi_factory.h"
#include "sdmpi_utils.h"
#include "sol/sol.hpp"
#include "ffmpeg.h"

using namespace mr::tio;
static const std::map<AVPixelFormat,mr::tio::SoftwareFrameFormat> av_tio_format_map={
    {AV_PIX_FMT_NONE,kSoftwareFormatNone},
    {AV_PIX_FMT_YUV420P,kSoftwareFormatI420},
    //{AV_PIX_FMT_YV12,kSoftwareFormatYV12},//ffmpeg not support
    {AV_PIX_FMT_NV12,kSoftwareFormatNV12},
    {AV_PIX_FMT_NV21,kSoftwareFormatNV21},
    {AV_PIX_FMT_YUV422P,kSoftwareFormatI422},
    {AV_PIX_FMT_NV16,kSoftwareFormatNV16},
    //{AV_PIX_FMT_NV61,kSoftwareFormatNV61},//ffmpeg not support
    {AV_PIX_FMT_YUYV422,kSoftwareFormatYUYV422},
    {AV_PIX_FMT_YVYU422,kSoftwareFormatYVYU422},
    {AV_PIX_FMT_UYVY422,kSoftwareFormatUYVY422},
    {AV_PIX_FMT_YUV444P,kSoftwareFormatI444},
    {AV_PIX_FMT_NV24,kSoftwareFormatNV24},
    {AV_PIX_FMT_NV42,kSoftwareFormatNV42},
    //{AV_PIX_FMT_YUV444,kSoftwareFormatYUV444},ffmpeg not support
    {AV_PIX_FMT_RGB24,kSoftwareFormatRGB24},
    {AV_PIX_FMT_BGR24,kSoftwareFormatBGR24},
    {AV_PIX_FMT_RGBA,kSoftwareFormatRGBA32},
    {AV_PIX_FMT_BGRA,kSoftwareFormatBGRA32},
    {AV_PIX_FMT_ARGB,kSoftwareFormatARGB32},
    {AV_PIX_FMT_ABGR,kSoftwareFormatABGR32},
    {AV_PIX_FMT_GRAY8,kSoftwareFormatGRAY8},
    {AV_PIX_FMT_GRAY8A,kSoftwareFormatGRAY8A}
};

static inline SoftwareFrame frame_to_tio (const AVFrame& av_frame){
    SoftwareFrame tio_frame;
    tio_frame.format = kSoftwareFormatNone;
    auto it = av_tio_format_map.find((AVPixelFormat)av_frame.format);
    if(it != av_tio_format_map.end())
        tio_frame.format = it->second;

    tio_frame.width = av_frame.width;
    tio_frame.height = av_frame.height;
    for(int index = 0; index < 4; index++){
        tio_frame.data[index] = av_frame.data[index];
        tio_frame.linesize[index] = av_frame.linesize[index];
    }
    return tio_frame;
}
#endif // CORE_INCLUDES_H
