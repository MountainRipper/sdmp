#include "video_decoder_rkmpp.h"
#if defined(HAS_ROCKCHIP_MPP)
#include "logger.h"
#include <rockchip/mpp_buffer.h>
#include <rockchip/rk_mpi.h>
#include <libdrm/drm_fourcc.h>
extern "C"{
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
}

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
    case AV_CODEC_ID_VC1:           return MPP_VIDEO_CodingVC1;
    case AV_CODEC_ID_MPEG2VIDEO:    return MPP_VIDEO_CodingMPEG2;
    case AV_CODEC_ID_MPEG4:         return MPP_VIDEO_CodingMPEG4;
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

namespace mr::sdmp {

COM_REGISTER_OBJECT(VideoDecoderRkmppFilter)

VideoDecoderRkmppFilter::VideoDecoderRkmppFilter()
{
    params_ = avcodec_parameters_alloc();
}

VideoDecoderRkmppFilter::~VideoDecoderRkmppFilter()
{
    avcodec_parameters_free(&params_);
    close_decoder();
}

int32_t VideoDecoderRkmppFilter::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_VIDEO,kInputPin);
    create_general_pin(AVMEDIA_TYPE_VIDEO,kOutputPin);
    GeneralFilter::initialize(graph,filter_values);
    return 0;
}

int32_t VideoDecoderRkmppFilter::process_command(const std::string &command, const Value& param)
{
    if(command == kGraphCommandSeek){
        RKMPPDecoder *decoder = (RKMPPDecoder *)decoder_ref->data;
        return decoder->mpi->reset(decoder->ctx);
        decode_param();
    }
    return GeneralFilter::process_command(command,param);
}

int32_t VideoDecoderRkmppFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    const auto& formats = sender_pin->formats();
    int index = -1;
    for(auto& format : formats){
        ++index;

        if( format.codec != AV_CODEC_ID_H264 &&
            format.codec != AV_CODEC_ID_HEVC &&
            format.codec != AV_CODEC_ID_VP9 &&
            format.codec != AV_CODEC_ID_VP9 &&
            format.codec != AV_CODEC_ID_VC1){
            continue;
        }
        int ret = open_decoder(format);
        if(ret < 0)
            continue;
        return index;
    }
    close_decoder();
    return -1;
}

int32_t VideoDecoderRkmppFilter::connect_chose_output_format(IPin* output_pin, int32_t index)
{
    return 0;
}

int32_t VideoDecoderRkmppFilter::receive(IPin* input_pin,FramePointer frame)
{
    if(status_ == kStatusEos)
        return kErrorFilterEos;

    if(frame == nullptr)
        return  kErrorInvalidParameter;

    if(frame->packet == nullptr){
        if(frame->flag & kFrameFlagEos){
            //decode eos frame
            decode_a_frame(frame);
            Value status((double)kStatusEos);
            set_property_async(kFilterPropertyStatus, status);
            deliver_eos_frame();
            return kSuccessed;
        }
        return kErrorInvalidParameter;
    }

    int ret = 0;
    bool is_annexb = CHECK_ANNEXB_START_CODE(frame->packet->data);
    if(bitstream_converter_ != nullptr && !is_annexb){
        bitstream_converter_->convert(frame->packet);
        do{
            auto new_pack = bitstream_converter_->pop();
            if(new_pack == nullptr)
                break;
            ret = decode_a_frame(new_pack);
            if(ret < 0)
                break;
        }while(true);
    }
    else{
        ret = decode_a_frame(frame);
    }
    return ret;
}


int32_t VideoDecoderRkmppFilter::requare(int32_t duration,const std::vector<PinIndex>& output_pins)
{
    if(status_ == kStatusEos)
        return kErrorFilterEos;

    return duration;
}

int32_t VideoDecoderRkmppFilter::property_changed(const std::string& name,Value& symbol)
{
    return 0;
}

int32_t VideoDecoderRkmppFilter::open_decoder(const Format& format)
{
    AVCodecParameters *param = (AVCodecParameters*)format.codec_parameters;
    close_decoder();

    avcodec_parameters_copy(params_,param);

    RKMPPDecoder *decoder = (RKMPPDecoder*)av_mallocz(sizeof(RKMPPDecoder));
    decoder_ref = av_buffer_create((uint8_t *)decoder, sizeof(*decoder), rkmpp_release_decoder,
                                                   NULL, AV_BUFFER_FLAG_READONLY);

    decoder->device_ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_DRM);
    av_hwdevice_ctx_init(decoder->device_ref);

    MppCodingType codec_rkmpp = rkmpp_get_codingtype(params_->codec_id);
    int ret = mpp_check_support_format(MPP_CTX_DEC,codec_rkmpp);
    if(MPP_SUCCESS != ret){
        return kErrorFormatNotAcceped;
    }
    ret = mpp_create(&decoder->ctx, &decoder->mpi);
    if(MPP_SUCCESS != ret){
        return ret;
    }
    ret = mpp_init(decoder->ctx, MPP_CTX_DEC, codec_rkmpp);
    if(MPP_SUCCESS != ret){
        return ret;
    }

    format_out_.type = AVMEDIA_TYPE_VIDEO;
    format_out_.codec = AV_CODEC_ID_RAWVIDEO;
    format_out_.format = AV_PIX_FMT_DRM_PRIME;
    format_out_.width = params_->width;
    format_out_.height = params_->height;
    format_out_.fps = format.fps;
    update_pin_format(kOutputPin,0,0,format_out_);

    if(params_->codec_id ==  AV_CODEC_ID_H264)
        bitstream_converter_ = std::shared_ptr<BitStreamConvert>(new BitStreamConvert("h264_mp4toannexb"));
    else if(params_->codec_id ==  AV_CODEC_ID_HEVC)
        bitstream_converter_ = std::shared_ptr<BitStreamConvert>(new BitStreamConvert("hevc_mp4toannexb"));
    else
        bitstream_converter_ = nullptr;

    if(bitstream_converter_)
        bitstream_converter_->create(param);

    decode_param();
    return 0;
}

int32_t VideoDecoderRkmppFilter::decode_a_frame(FramePointer sdp_frame)
{
    int ret = 0;
    RK_U32 pkt_done = 0;
    MppPacket mpp_packet = nullptr;
    auto packet = sdp_frame->packet;
    bool mEos = ((packet == nullptr) || (sdp_frame->flag == kFrameFlagEos));
    if(!mEos){
        mpp_packet_init(&mpp_packet,packet->data,packet->size);
        mpp_packet_set_pts(mpp_packet,packet->pts);
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
                    MP_LOG_DEAULT("decode_get_frame failed too much time.");
                }

                if (ret != MPP_OK) {
                    MP_LOG_DEAULT("decode_get_frame failed ret {}.", ret);
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
                    MP_LOG_DEAULT("found last frame.");
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

int32_t VideoDecoderRkmppFilter::decode_param()
{
    RKMPPDecoder *decoder = (RKMPPDecoder *)decoder_ref->data;
    // create the MPP packet
    MppPacket pack;
    int ret = 0;
    if(bitstream_converter_){
        auto param_out = bitstream_converter_->params_out();
        mpp_packet_init(&pack, param_out->extradata,param_out->extradata_size);
    }
    else{
        mpp_packet_init(&pack, params_->extradata, params_->extradata_size);
    }
    if (ret != MPP_OK) {
        return AVERROR_UNKNOWN;
    }
    mpp_packet_set_pts(pack, 0);
    ret = decoder->mpi->decode_put_packet(decoder->ctx, pack);
    if (ret != MPP_OK) {
        if (ret == MPP_ERR_BUFFER_FULL) {
            MP_LOG_DEAULT( "Buffer full writing {} bytes to decoder",params_->extradata_size);
            ret = AVERROR(EAGAIN);
        } else
            ret = AVERROR_UNKNOWN;
    }    

    mpp_packet_deinit(&pack);
    return 0;
}

int32_t VideoDecoderRkmppFilter::export_frame(MppFrame* mppframe_pointer)
{
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

        auto new_frame = Frame::make_frame(frame);
        new_frame->releaser = [](AVFrame* frame,AVPacket*){
            AVDRMFrameDescriptor* frame_desc = (AVDRMFrameDescriptor*)frame->data[0];
            //MP_LOG_DEAULT "DRM FRAME:{} Released ",frame_desc->objects[0].fd);
            av_frame_free(&frame);
        };
        //MP_LOG_DEAULT "DRM FRAME:{} emit ",desc->objects[0].fd);
        get_pin(kOutputPin,0)->deliver(new_frame);
    }
    return 0;
}

int32_t VideoDecoderRkmppFilter::close_decoder()
{
    if(decoder_ref){
        av_buffer_unref(&decoder_ref);
        decoder_ref = nullptr;
    }
    return 0;
}


}//HAS_ROCKCHIP_MPP

#endif

