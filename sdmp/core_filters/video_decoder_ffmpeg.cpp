#include "video_decoder_ffmpeg.h"
extern "C"{
#include <libavutil/hwcontext.h>
#if defined (__linux) && !defined (__ANDROID__)
#include <libavutil/hwcontext_vaapi.h>
#include <libavutil/hwcontext_drm.h>
#endif
}

namespace mr::sdmp {

#if defined (HAS_ROCKCHIP_MPP)
FramePointer avframe_transfer_drm(AVFrame* frame){
    FramePointer sdp_frame;
    if(frame == nullptr)
        return sdp_frame;
    if( frame->format != AV_PIX_FMT_DRM_PRIME)
        return sdp_frame;

    AVFrame *sw_frame = av_frame_alloc();
//    int err = av_hwframe_transfer_data(sw_frame, frame, 0);
//    if (err < 0) {
//        return sdp_frame;
//    }
    sw_frame->pts = frame->pts;
    sdp_frame = Frame::make_frame(sw_frame);
    sdp_frame->releaser = sdp_frame_free_frame_releaser;
    return sdp_frame;
}
#endif

FramePointer avframe_transfer_ffmpeg_inner(AVFrame* frame){
    FramePointer sdp_frame;
    if(frame == nullptr)
        return sdp_frame;
    if( frame->format != AV_PIX_FMT_VAAPI       &&
        frame->format != AV_PIX_FMT_DXVA2_VLD   &&
        frame->format != AV_PIX_FMT_D3D11VA_VLD &&
        frame->format != AV_PIX_FMT_D3D11)
        return sdp_frame;

    AVFrame *sw_frame = av_frame_alloc();
    int err = av_hwframe_transfer_data(sw_frame, frame, 0);
    if (err < 0) {
        return sdp_frame;
    }
    sw_frame->pts = frame->pts;
    sdp_frame = Frame::make_frame(sw_frame);
    sdp_frame->releaser = sdp_frame_free_frame_releaser;
    return sdp_frame;
}

std::map<int,FrameSoftwareTransfer> hardware_frame_transfers = {
#if defined (HAS_ROCKCHIP_MPP)
  //{AV_PIX_FMT_DRM_PRIME,avframe_transfer_drm},
#endif
  {AV_PIX_FMT_VAAPI,avframe_transfer_ffmpeg_inner},
  {AV_PIX_FMT_DXVA2_VLD,avframe_transfer_ffmpeg_inner},
  {AV_PIX_FMT_D3D11VA_VLD,avframe_transfer_ffmpeg_inner},
  {AV_PIX_FMT_D3D11,avframe_transfer_ffmpeg_inner}
};

COM_REGISTER_OBJECT(VideoDecoderFFmpegFilter)

VideoDecoderFFmpegFilter::VideoDecoderFFmpegFilter()
{

}

VideoDecoderFFmpegFilter::~VideoDecoderFFmpegFilter()
{
    close_decoder();
}

int32_t VideoDecoderFFmpegFilter::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_VIDEO,kInputPin);
    create_general_pin(AVMEDIA_TYPE_VIDEO,kOutputPin);
    GeneralFilter::initialize(graph,filter_values);
    decode_to_frames_ = properties_["framesPreDecode"].as_int64();
    return 0;
}

int32_t VideoDecoderFFmpegFilter::process_command(const std::string &command, const Value& param)
{
    GeneralFilter::process_command(command,param);
    if(command == kGraphCommandPause){
        pause_wait_.request_and_wait(&decode_condition_);
    }
    else if(command == kGraphCommandSeek || command == kGraphCommandStop){
        std::lock_guard<std::mutex> lock(decode_mutex_);
        current_decode_frames_ = 0;
        has_decode_frames_ = 0;
        decode_to_frames_ = properties_["framesPreDecode"].as_int64();
        received_frames_ = std::queue<FramePointer>();
        if(decoder_)
            avcodec_flush_buffers(decoder_);
    }
    else if(command == kGraphCommandPlay){
        pause_wait_.reset();
    }
    return 0;
}

int32_t VideoDecoderFFmpegFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    const auto& formats = sender_pin->formats();
    int index = -1;
    for(auto& format : formats){
        ++index;
        int ret = open_decoder(format);
        if(ret < 0)
            continue;

        quit_decode_ = false;
        decode_thread_ = std::thread(&VideoDecoderFFmpegFilter::decode_proc,this);
        return index;
    }
    close_decoder();
    return -1;
}

int32_t VideoDecoderFFmpegFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{    
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t VideoDecoderFFmpegFilter::receive(IPin* input_pin,FramePointer frame)
{
    std::lock_guard<std::mutex> lock(decode_mutex_);
    if(status_ == kStatusStoped)
        return 0;
//    if(frame->packet)
//        MP_LOG_DEAULT("Receive:{}",frame->packet->flags & AV_PKT_FLAG_KEY);
    received_frames_.push(frame);
    return 0;
}


int32_t VideoDecoderFFmpegFilter::requare(int32_t duration,const std::vector<PinIndex>& output_pins)
{
    if(status_ == kStatusEos)
        return kErrorFilterEos;

    std::lock_guard<std::mutex> lock(decode_mutex_);
    double frame_interval = (1000.0/format_out_.fps);
    int64_t need_decode_frames = round(duration / frame_interval);
    if(need_decode_frames > 0){
        decode_to_frames_ = has_decode_frames_ + need_decode_frames;
        if(decode_to_frames_ > current_decode_frames_)
             decode_condition_.notify_one();        
    }

    int64_t cache_keep = properties_["framesKeepCache"];
    int64_t frams_has = received_frames_.size();
    int64_t frames_exsit = (frams_has - need_decode_frames - cache_keep);//maybe -x
    if(frames_exsit >= 0){
        return 0;
    }
    return -frames_exsit*frame_interval;
}

int32_t VideoDecoderFFmpegFilter::property_changed(const std::string& name,Value& symbol)
{
    return 0;
}

AVPixelFormat VideoDecoderFFmpegFilter::hardware_format_requare()
{
    return hw_format_;
}

int32_t VideoDecoderFFmpegFilter::open_decoder(const Format &format)
{
    close_decoder();

    if(format.type != AVMEDIA_TYPE_VIDEO)
        return -1;
    if(format.codec_parameters == nullptr)
        return -2;
    if(format.width == 0 || format.height == 0)
        return -3;

    codec_ = nullptr;
    std::string codec_name_manually = properties_["decoderName"];
    if(codec_name_manually.size()){
        //try open codec set by user
        codec_ = avcodec_find_decoder_by_name(codec_name_manually.c_str());
        MP_LOG_DEAULT("ffmpeg decoder try open codec:{} get:{}",codec_name_manually.c_str(),codec_?codec_->long_name:"null");
    }
    if(codec_ == nullptr){
        codec_ = avcodec_find_decoder((AVCodecID)format.codec);
    }
    if(codec_ == nullptr)
        return -4;

    decoder_ = avcodec_alloc_context3(codec_);
    if(decoder_ == nullptr)
        return -5;
    decoder_->opaque = this;

    int ret = 0;
    AVCodecParameters* params = (AVCodecParameters*)format.codec_parameters;
    ret = avcodec_parameters_to_context(decoder_,params);
    if(ret < 0)
        return -6;

    //decoder_->time_base = {format.timebase.numerator,format.timebase.denominator};
    ret = try_open_hardware_context();

    ret = avcodec_open2(decoder_,codec_,nullptr);
    if(ret < 0){
        char error_msg[128];
        MP_ERROR("create ffmpeg decoder failed:{}",av_make_error_string(error_msg,128,ret));
        return -7;
    }

    format_out_.type = AVMEDIA_TYPE_VIDEO;
    format_out_.codec = AV_CODEC_ID_RAWVIDEO;
    format_out_.format = decoder_->pix_fmt;
    format_out_.width = decoder_->width;
    format_out_.height = decoder_->height;
    format_out_.timebase = format.timebase;
    format_out_.fps = format.fps;    
    format_out_.bitrate = av_image_get_buffer_size(decoder_->pix_fmt,decoder_->width,decoder_->height,1)*8*format.fps;

    if(hw_format_ != AV_PIX_FMT_NONE){
        format_out_.format = hw_format_;
    }

    update_pin_format(kInputPin,0,0,format);
    update_pin_format(kOutputPin,0,0,format_out_);
    return 0;
}

int32_t VideoDecoderFFmpegFilter::try_open_hardware_context()
{
    std::string hardware_api_manually = properties_["hardwareApi"];
    MP_INFO("Try to use hardware decode api:{}",hardware_api_manually);

    if(hardware_api_manually.empty())
        return 0;


    std::string os = sdp_os();
    if(hardware_api_manually == "auto"){
        if(os == "linux"){
            hardware_api_manually = "vaapi";
        }
        else if(os == "windows"){
            hardware_api_manually = "dxva2";
        }
        else if(os == "android"){
            hardware_api_manually = "mediacodec";
        }
        else if(os == "ios"){
            hardware_api_manually = "videotoolbox";
        }
    }
    AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;
    for(int type = AV_HWDEVICE_TYPE_VDPAU; type <= AV_HWDEVICE_TYPE_MEDIACODEC; type++){
        auto type_name = av_hwdevice_get_type_name((AVHWDeviceType)type);
        if(hardware_api_manually.find(type_name) != std::string::npos){
            hw_device_type = (AVHWDeviceType)type;
            hardware_api_manually = type_name;
            break;
        }
    }
    if(hw_device_type == AV_HWDEVICE_TYPE_NONE){
        MP_WARN("WARNNING: hardware api:{} invalid",hardware_api_manually);
        return -1;
    }

    //find if hardware api support this codec
    hw_format_ = AV_PIX_FMT_NONE;
    for (int i = 0 ; ; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec_, i);
        if (!config) {
            break;
        }
        if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) && (config->device_type == hw_device_type)) {
            hw_format_ = config->pix_fmt;
            break;
        }
    }

    if( hw_format_ == AV_PIX_FMT_NONE){
        MP_WARN("WARNNING: hardware api:{} not support codec:{}",hardware_api_manually,avcodec_get_name(codec_->id));
        return -3;
    }
    else{
        //create hardware device context
        sol::state& vm = *graph_->vm();
        hw_driver_ = vm["hostFeature"]["graphicDriver"].get_or(std::string());
        AVDictionary *options = nullptr;
        if(hw_driver_ == "glx")
            av_dict_set(&options,"connection_type","x11",0);

        int err = av_hwdevice_ctx_create(&hw_device_ctx_, hw_device_type,NULL, options, 0);

        if(options)
            av_dict_free(&options);

        if (err < 0) {
            char error_msg[128];
            MP_WARN("WARNNING: hardware api:{} can't create context, driver:{} error:{}",hardware_api_manually,hw_driver_,
                        av_make_error_string(error_msg,128,err));
            return err;
        }

        decoder_->get_format = [](AVCodecContext *ctx,const enum AVPixelFormat *pix_fmts) -> AVPixelFormat{
            VideoDecoderFFmpegFilter* decoder = (VideoDecoderFFmpegFilter*)ctx->opaque;
            AVPixelFormat hw_format = decoder->hardware_format_requare();
            const enum AVPixelFormat *format_walker;
            for (format_walker = pix_fmts; *format_walker != AV_PIX_FMT_NONE; format_walker++) {
                if (*format_walker == hw_format)
                    return *format_walker;
            }
            return AV_PIX_FMT_NONE;
        };
        MP_INFO("hardware api:{} driver:{} inited successed",hardware_api_manually,hw_driver_);
        decoder_->hw_device_ctx = av_buffer_ref(hw_device_ctx_);
        decoder_->hw_frames_ctx = av_hwframe_ctx_alloc(decoder_->hw_device_ctx);
    }
    return 0;
}

int32_t VideoDecoderFFmpegFilter::close_decoder()
{
    quit_decode_ = true;
    hw_driver_ = "";
    decode_condition_.notify_all();
    if(decode_thread_.joinable())
        decode_thread_.join();

    if(decoder_){
        avcodec_free_context(&decoder_);
        decoder_ = nullptr;
    }
    if(hw_device_ctx_){
        av_buffer_unref(&hw_device_ctx_);
        hw_device_ctx_ = nullptr;
    }
    update_pin_format(kInputPin,0,0,sdmp::Format());
    update_pin_format(kOutputPin,0,0,sdmp::Format());
    return 0;
}

int32_t VideoDecoderFFmpegFilter::decode_proc()
{
    while (!quit_decode_) {
        std::queue<FramePointer> frames;
        {
            pause_wait_.check();//maybe decode lasted frame and wait
            std::unique_lock<std::mutex> lock(decode_mutex_);
            decode_condition_.wait(lock);
            if(quit_decode_)
                break;
            if(pause_wait_.check())
                continue;

            if(received_frames_.empty())
                continue;

            if(current_decode_frames_ >= decode_to_frames_)
                continue;

            while (received_frames_.size()) {
                frames.push(received_frames_.front());
                received_frames_.pop();
                ++current_decode_frames_;
                if(current_decode_frames_ >= decode_to_frames_)
                    break;
            }
            //MP_LOG_DEAULT("decoder has cached frames:{} decode:{}",received_frames_.size(),frames.size());
        }

        while (frames.size()) {
            if(pause_wait_.check())
                break;
            has_decode_frames_++;
            decode_a_frame(frames.front());
            frames.pop();
        }
    }
    return 0;
}

int32_t VideoDecoderFFmpegFilter::decode_a_frame(FramePointer frame)
{
    if(decoder_ == nullptr)
        return -1;
    if(frame == nullptr)
        return -2;

    std::unique_lock<std::mutex> lock(decode_mutex_);
    int ret = avcodec_send_packet(decoder_,frame->packet);
    if(ret < 0)
        return ret;

    do{
        AVFrame* decode_frame = av_frame_alloc();
        ret = avcodec_receive_frame(decoder_,decode_frame);
        if (ret < 0){
            av_frame_free(&decode_frame);
            if(ret == AVERROR(EAGAIN)){
                break;
            }
            else {
                break;
            }
        }
        if (ret >= 0){

            FramePointer new_frame;
            if(decode_frame->format == AV_PIX_FMT_VAAPI){
                if(hw_driver_ == "egl"){
                    AVFrame* drm_frame = av_frame_alloc();
                    drm_frame->format = AV_PIX_FMT_DRM_PRIME;
                    drm_frame->pts = decode_frame->pts;
                    ret = av_hwframe_map(drm_frame,decode_frame,AV_HWFRAME_MAP_WRITE|AV_HWFRAME_MAP_READ);
                    if(ret < 0){
                        av_frame_free(&drm_frame);
                    }
                    else{
                        new_frame = Frame::make_frame(drm_frame);
                        av_frame_free(&decode_frame);
                    }
                }
                else{
                    //transfer to memory inmediatlly,not transfer when renderer(vaapi frame cache is limited)
                    new_frame = avframe_transfer_ffmpeg_inner(decode_frame);
                    av_frame_free(&decode_frame);
                }
            }

            if(!new_frame){
                new_frame = Frame::make_frame(decode_frame);
                auto it = hardware_frame_transfers.find(decode_frame->format);
                if(it!=hardware_frame_transfers.end())
                    new_frame->transfer = it->second;
            }

            new_frame->releaser = sdp_frame_free_frame_releaser;

            get_pin(kOutputPin,0)->deliver(new_frame);
        }

    }while (true);


    if(frame->packet == nullptr){
        if(frame->flag & kFrameFlagEos){
            Value status((double)kStatusEos);
            set_property_async(kFilterPropertyStatus, status);
            deliver_eos_frame();
        }
    }
    return 0;
}

}


