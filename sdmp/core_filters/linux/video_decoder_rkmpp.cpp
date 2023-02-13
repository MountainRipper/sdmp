#include <unistd.h>
#include <pthread.h>
#include <drm_fourcc.h>
extern "C"{
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
}

#include "video_decoder_rkmpp.h"

#define cxx_log(fmt,...)\
            do {\
                printf(fmt,##__VA_ARGS__);\
            } while(0)

struct RKMPPFrameContext{
    MppFrame frame = nullptr;
    AVBufferRef *decoder_ref = nullptr;
} ;

struct RKMPPDecoder{
    MppCtx ctx = nullptr;
    MppApi *mpi = nullptr;
    MppBufferGroup frame_group = nullptr;

    AVBufferRef *frames_ref = nullptr;
    AVBufferRef *device_ref = nullptr;
} ;

static MppCodingType rkmpp_get_codingtype(AVCodecID codec)
{
    switch (codec) {
    case AV_CODEC_ID_H264:          return MPP_VIDEO_CodingAVC;
    case AV_CODEC_ID_HEVC:          return MPP_VIDEO_CodingHEVC;
    case AV_CODEC_ID_VP8:           return MPP_VIDEO_CodingVP8;
    case AV_CODEC_ID_VP9:           return MPP_VIDEO_CodingVP9;
    default:                        return MPP_VIDEO_CodingUnused;
    }
}

static uint32_t rkmpp_get_frameformat(MppFrameFormat mppformat)
{
    switch (mppformat) {
    case MPP_FMT_YUV420SP:          return DRM_FORMAT_NV12;
#ifdef DRM_FORMAT_NV12_10
    case MPP_FMT_YUV420SP_10BIT:    return DRM_FORMAT_NV12_10;
#endif
    default:                        return 0;
    }
}
static void rkmpp_release_frame(void *opaque, uint8_t *data)
{
    AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)data;
    AVBufferRef *framecontextref = (AVBufferRef *)opaque;
    RKMPPFrameContext *framecontext = (RKMPPFrameContext *)framecontextref->data;

    mpp_frame_deinit(&framecontext->frame);
    av_buffer_unref(&framecontext->decoder_ref);
    av_buffer_unref(&framecontextref);

    av_free(desc);
}

static void rkmpp_release_decoder(void *opaque, uint8_t *data)
{
    RKMPPDecoder *decoder = (RKMPPDecoder *)data;

    if (decoder->mpi) {
        decoder->mpi->reset(decoder->ctx);
        mpp_destroy(decoder->ctx);
        decoder->ctx = NULL;
    }

    if (decoder->frame_group) {
        mpp_buffer_group_put(decoder->frame_group);
        decoder->frame_group = NULL;
    }

    av_buffer_unref(&decoder->frames_ref);
    av_buffer_unref(&decoder->device_ref);

    av_free(decoder);
}


VideoDecoderRkmpp::VideoDecoderRkmpp()
{

}

VideoDecoderRkmpp::~VideoDecoderRkmpp()
{
    av_buffer_unref(&decoder_ref);
}

int32_t VideoDecoderRkmpp::pixel_format()
{
    return AV_PIX_FMT_DRM_PRIME;
}

int32_t VideoDecoderRkmpp::stream_type()
{
    return kH264StreamTypeAnnexB;
}

int32_t VideoDecoderRkmpp::init_decoder(const std::vector<DecoderInitParams> &params, VideoCodecType codec_id, uint32_t width, uint32_t height, uint16_t rotate, uint16_t fps, uint32_t kbps)
{
    RKMPPDecoder *decoder = (RKMPPDecoder*)av_mallocz(sizeof(RKMPPDecoder));
    decoder_ref = av_buffer_create((uint8_t *)decoder, sizeof(*decoder), rkmpp_release_decoder,
                                                   NULL, AV_BUFFER_FLAG_READONLY);

    decoder->device_ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_DRM);
    av_hwdevice_ctx_init(decoder->device_ref);

cxx_log("1111\n");
    MppCodingType codec_rkmpp = rkmpp_get_codingtype((AVCodecID)codec_id);
    int ret = mpp_check_support_format(MPP_CTX_DEC,codec_rkmpp);
    if(MPP_SUCCESS != ret){
        return ret;
    }
cxx_log("2222\n");
    ret = mpp_create(&decoder->ctx, &decoder->mpi);
    if(MPP_SUCCESS != ret){
        return ret;
    }
cxx_log("3333\n");
    ret = mpp_init(decoder->ctx, MPP_CTX_DEC, codec_rkmpp);
    if(MPP_SUCCESS != ret){
        return ret;
    }
cxx_log("4444\n");
    params_ = params;

    return 0;
}

int32_t VideoDecoderRkmpp::flush()
{
    if(!decoder_ref)
        return -1;
    RKMPPDecoder *decoder = (RKMPPDecoder *)decoder_ref->data;
    return decoder->mpi->reset(decoder->ctx);
    first_frame_ = true;
    return 0;
}

int32_t VideoDecoderRkmpp::decode(VideoDecodeData *packet, int32_t drop_pts)
{
    if(first_frame_){
        first_frame_ = false;
        decode_params();
    }
    int ret = 0;
    RK_U32 pkt_done = 0;
    MppPacket mpp_packet = nullptr;
    bool mEos = (packet == nullptr);
    if(!mEos){
        packet->convert_to_annexb();
        mpp_packet_init(&mpp_packet,packet->data_.get(),packet->size_);
        mpp_packet_set_pts(mpp_packet,packet->pts_);
    }
    else{
        mpp_packet_init(&mpp_packet,NULL,0);
        mpp_packet_set_pts(mpp_packet,0);
        mpp_packet_set_eos(mpp_packet);
    }

    RKMPPDecoder *decoder = (RKMPPDecoder *)decoder_ref->data;
    MppFrame frame;
    MppFrame *srcFrm = &frame;

    do {
        RK_S32 times = 5;

        if (!pkt_done) {
            ret = decoder->mpi->decode_put_packet(decoder->ctx, mpp_packet);
            if (ret == MPP_OK)
                pkt_done = 1;
        }

        do {
            RK_S32 get_frm = 0;
            RK_U32 frm_eos = 0;

            try_again:
                ret = decoder->mpi->decode_get_frame(decoder->ctx, srcFrm);
                if (ret == MPP_ERR_TIMEOUT) {
                    if (times > 0) {
                        times--;
                        usleep(2000);
                        goto try_again;
                    }
                    cxx_log("decode_get_frame failed too much time.\n");
                }

                if (ret != MPP_OK) {
                    cxx_log("decode_get_frame failed ret %d.\n", ret);
                    break;
                }

                if (*srcFrm) {
                    frm_eos = mpp_frame_get_eos(*srcFrm);

                    if (mpp_frame_get_info_change(*srcFrm)) {
                        RK_U32 width = mpp_frame_get_width(*srcFrm);
                        RK_U32 height = mpp_frame_get_height(*srcFrm);
                        RK_U32 hor_stride = mpp_frame_get_hor_stride(*srcFrm);
                        RK_U32 ver_stride = mpp_frame_get_ver_stride(*srcFrm);

                        ret = mpp_buffer_group_get_internal(&decoder->frame_group, MPP_BUFFER_TYPE_DRM);
                        if (ret) {
                            break;
                        }

                        decoder->mpi->control(decoder->ctx, MPP_DEC_SET_EXT_BUF_GROUP, decoder->frame_group);
                        decoder->mpi->control(decoder->ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);

                        av_buffer_unref(&decoder->frames_ref);
                        decoder->frames_ref = av_hwframe_ctx_alloc(decoder->device_ref);
                        AVHWFramesContext* hwframes = (AVHWFramesContext*)decoder->frames_ref->data;

                        auto mppformat = mpp_frame_get_fmt(*srcFrm);
                        auto drmformat = rkmpp_get_frameformat(mppformat);
                        hwframes->format    = AV_PIX_FMT_DRM_PRIME;
                        hwframes->sw_format = drmformat == DRM_FORMAT_NV12 ? AV_PIX_FMT_NV12 : AV_PIX_FMT_NONE;
                        hwframes->width     = width;
                        hwframes->height    = height;
                        ret = av_hwframe_ctx_init(decoder->frames_ref);

                        mpp_frame_deinit(srcFrm);
                    } else {
                        export_frame(srcFrm);
                    }
                    *srcFrm = NULL;
                    get_frm = 1;
                }

                if (mEos && pkt_done && !frm_eos) {
                    usleep(10000);
                    continue;
                }

                if (frm_eos) {
                    cxx_log("found last frame.\n");
                    break;
                }

                if (get_frm)
                    continue;
                break;
        } while (1);

        if (pkt_done)
            break;
        /*
         * why sleep here?
         * mpp has a internal input pakcet list,if it is full, wait here 3ms to
         * wait internal list isn't full.
         */
        usleep(3000);
    } while (1);

    mpp_packet_deinit(&mpp_packet);
    return 0;
}

int32_t VideoDecoderRkmpp::set_property(const std::string &key, const std::string &value)
{
    return 0;
}

int32_t VideoDecoderRkmpp::export_frame(MppFrame* mppframe_pointer)
{
    if(event_ == nullptr){
        mpp_frame_deinit(mppframe_pointer);
    }
    MppFrame mppframe = *mppframe_pointer;
    RKMPPDecoder *decoder = (RKMPPDecoder *)decoder_ref->data;

    MppBuffer buffer = NULL;
    AVDRMFrameDescriptor *desc = NULL;
    AVDRMLayerDescriptor *layer = NULL;
    RKMPPFrameContext *framecontext = NULL;
    AVBufferRef *framecontextref = NULL;
    MppFrameFormat mppformat = MPP_FMT_YUV420P;
    uint32_t drmformat = DRM_FORMAT_NV12;
    int mode = 0;
    int ret  = 0;

    AVFrame* frame = av_frame_alloc();
    // setup general frame fields
    frame->format           = AV_PIX_FMT_DRM_PRIME;
    frame->width            = mpp_frame_get_width(mppframe);
    frame->height           = mpp_frame_get_height(mppframe);
    frame->pts              = mpp_frame_get_pts(mppframe);
    frame->color_range      = (AVColorRange)mpp_frame_get_color_range(mppframe);
    frame->color_primaries  = (AVColorPrimaries)mpp_frame_get_color_primaries(mppframe);
    frame->color_trc        = (AVColorTransferCharacteristic)mpp_frame_get_color_trc(mppframe);
    frame->colorspace       = (AVColorSpace)mpp_frame_get_colorspace(mppframe);

    mode = mpp_frame_get_mode(mppframe);
    frame->interlaced_frame = ((mode & MPP_FRAME_FLAG_FIELD_ORDER_MASK) == MPP_FRAME_FLAG_DEINTERLACED);
    frame->top_field_first  = ((mode & MPP_FRAME_FLAG_FIELD_ORDER_MASK) == MPP_FRAME_FLAG_TOP_FIRST);

    mppformat = mpp_frame_get_fmt(mppframe);
    drmformat = rkmpp_get_frameformat(mppformat);

    // now setup the frame buffer info
    buffer = mpp_frame_get_buffer(mppframe);
    if (buffer) {
        desc = (AVDRMFrameDescriptor*)av_mallocz(sizeof(AVDRMFrameDescriptor));

        desc->nb_objects = 1;
        desc->objects[0].fd = mpp_buffer_get_fd(buffer);
        desc->objects[0].size = mpp_buffer_get_size(buffer);

        desc->nb_layers = 1;
        layer = &desc->layers[0];
        layer->format = drmformat;
        layer->nb_planes = 2;

        layer->planes[0].object_index = 0;
        layer->planes[0].offset = 0;
        layer->planes[0].pitch = mpp_frame_get_hor_stride(mppframe);

        layer->planes[1].object_index = 0;
        layer->planes[1].offset = layer->planes[0].pitch * mpp_frame_get_ver_stride(mppframe);
        layer->planes[1].pitch = layer->planes[0].pitch;

        // we also allocate a struct in buf[0] that will allow to hold additionnal information
        // for releasing properly MPP frames and decoder
        framecontextref = (AVBufferRef*)av_buffer_allocz(sizeof(*framecontext));

        // MPP decoder needs to be closed only when all frames have been released.
        framecontext = (RKMPPFrameContext *)framecontextref->data;
        framecontext->decoder_ref = av_buffer_ref(decoder_ref);
        framecontext->frame = mppframe;


        frame->data[0]  = (uint8_t *)desc;
        frame->buf[0]   = av_buffer_create((uint8_t *)desc, sizeof(*desc), rkmpp_release_frame,
                                           framecontextref, AV_BUFFER_FLAG_READONLY);

        frame->hw_frames_ctx = av_buffer_ref(decoder->frames_ref);


        std::shared_ptr<VideoRenderFrame> render_frame = std::shared_ptr<VideoRenderFrame>(new VideoRenderFrame());
        render_frame->width  = frame->width ;
        render_frame->height = frame->height;
        render_frame->rotate = 0;
        render_frame->pts_ = frame->pts;
        render_frame->format = AV_PIX_FMT_DRM_PRIME;
        render_frame->drm_fd = desc->objects[0].fd;
        render_frame->inner_frame = frame;

        render_frame->release_function = [](AVFrame* frame){
//            AVDRMFrameDescriptor* frame_desc = (AVDRMFrameDescriptor*)frame->data[0];
//            fprintf(stderr, "DRM FRAME:%d Released \n",frame_desc->objects[0].fd);
            av_frame_free(&frame);
        };
       // fprintf(stderr, "%p DRM FRAME:%d DECODED \n",this,desc->objects[0].fd);
        event_->on_video_decode(render_frame,render_frame->width,render_frame->height,frame->pts,frame->pts);
    }
    return 0;
}

int32_t VideoDecoderRkmpp::decode_params()
{
    int ret = 0;
    RKMPPDecoder *decoder = (RKMPPDecoder *)decoder_ref->data;
    if(params_.size() > 0){
        std::vector<const uint8_t*> datas;
        std::vector<uint32_t> sizes;
        for(auto& param : params_){
            datas.push_back(param.data());
            sizes.push_back(param.size());
        }
        VideoDecodeData packet = VideoDecodeData::h264_combin_with_startcode(datas,sizes);

        MppPacket pack;

        // create the MPP packet
        ret = mpp_packet_init(&pack, packet.data_.get(), packet.size_);
        if (ret != MPP_OK) {
            return AVERROR_UNKNOWN;
        }
        mpp_packet_set_pts(pack, 0);
        ret = decoder->mpi->decode_put_packet(decoder->ctx, pack);
        if (ret != MPP_OK) {
            if (ret == MPP_ERR_BUFFER_FULL) {
                av_log(decoder->ctx, AV_LOG_DEBUG, "Buffer full writing %d bytes to decoder\n", packet.size_);
                ret = AVERROR(EAGAIN);
            } else
                ret = AVERROR_UNKNOWN;
        }
        else
            av_log(decoder->ctx, AV_LOG_DEBUG, "Wrote %d bytes to decoder\n", packet.size_);

        mpp_packet_deinit(&pack);
    }
    return ret;
}
