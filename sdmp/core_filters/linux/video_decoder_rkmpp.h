#ifndef VIDEO_DECODER_RKMPP_H
#define VIDEO_DECODER_RKMPP_H

#include "codec_interface.h"
#include "ffmpeg_include.h"
#include <rockchip/mpp_buffer.h>
#include <rockchip/rk_mpi.h>
typedef void* MppCtx;
typedef void* MppParam;
class VideoDecoderRkmpp : public IVideoDecoder
{
public:
    VideoDecoderRkmpp();
    virtual ~VideoDecoderRkmpp();
    // IVideoDecoder interface
public:
    virtual int32_t pixel_format();
    virtual int32_t stream_type();
    virtual int32_t init_decoder(const std::vector<DecoderInitParams> &params, VideoCodecType codec_id, uint32_t width, uint32_t height, uint16_t rotate, uint16_t fps, uint32_t kbps);
    virtual int32_t flush();
    virtual int32_t decode(VideoDecodeData *packet,int32_t drop_pts);
    virtual int32_t set_property(const std::string &key, const std::string &value);
private:
    int32_t export_frame(MppFrame *mppframe_pointer);
    int32_t decode_params();
private:
    std::vector<DecoderInitParams> params_;
    AVBufferRef *decoder_ref = nullptr;
    bool        first_frame_ = true;
};

#endif // VIDEO_DECODER_RKMPP_H
