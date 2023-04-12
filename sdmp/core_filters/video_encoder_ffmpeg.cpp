#include "video_encoder_ffmpeg.h"

namespace mr::sdmp{

COM_REGISTER_OBJECT(VideoEncoderFFmpegFilter)

VideoEncoderFFmpegFilter::VideoEncoderFFmpegFilter()
{

}

VideoEncoderFFmpegFilter::~VideoEncoderFFmpegFilter()
{
    close_encoder();
}

int32_t VideoEncoderFFmpegFilter::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_VIDEO,kInputPin);
    create_general_pin(AVMEDIA_TYPE_VIDEO,kOutputPin);
    return GeneralFilter::initialize(graph,filter_values);
}


int32_t sdmp::VideoEncoderFFmpegFilter::process_command(const std::string &command, const Value &param)
{
    GeneralFilter::process_command(command,param);
    if(command == kGraphCommandPause){
        pause_wait_.request_and_wait(&encode_condition_);
    }
    else if(command == kGraphCommandStop){
        close_encoder();
    }
    else if(command == kGraphCommandPlay){
        pause_wait_.reset();
    }
    return 0;
}

int32_t sdmp::VideoEncoderFFmpegFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    const auto& formats = sender_pin->formats();
    int index = -1;
    for(auto& item : formats){
        ++index;
        int32_t ret = open_encoder(item);
        if(ret < 0)
            continue;

        quit_encode_ = false;
        encode_thread_ = std::thread(&VideoEncoderFFmpegFilter::encode_proc,this);
        return index;
    }
    close_encoder();
    return -1;
}

int32_t sdmp::VideoEncoderFFmpegFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t sdmp::VideoEncoderFFmpegFilter::receive(IPin *input_pin, FramePointer frame)
{
    (void)input_pin;
    frames_.push(frame);
    encode_condition_.notify_one();
    return 0;
}

int32_t sdmp::VideoEncoderFFmpegFilter::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    (void)duration;
    (void)output_pins;
    return duration;
}

void VideoEncoderFFmpegFilter::encode_proc()
{
    std::queue<FramePointer> frames;
    while (!quit_encode_) {
        {
            pause_wait_.check();//maybe decode lasted frame and wait
            std::unique_lock<std::mutex> lock(encode_mutex_);
            encode_condition_.wait(lock);
            if(quit_encode_)
                break;
            if(pause_wait_.check())
                continue;

            if(frames_.size() == 0)
                continue;

            frames_.move(frames);
        }

        while (frames.size()) {
            if(pause_wait_.check())
                break;
            encode_a_frame(frames.front());
            frames.pop();
        }
    }
    FramePointer flush_frame = Frame::make_frame(nullptr);
    encode_a_frame(flush_frame);

    if(codec_context){
        avcodec_free_context(&codec_context);
        codec_context = nullptr;
    }
    if(parameters_){
        avcodec_parameters_free(&parameters_);
        parameters_ = nullptr;
    }
}


int32_t VideoEncoderFFmpegFilter::open_encoder(const Format& format)
{
    close_encoder();

    const AVCodec* codec = nullptr;
    auto encoderName = properties_["encoderName"].as_string();
    if(!encoderName.empty()){
        codec = avcodec_find_encoder_by_name(encoderName.c_str());
    }

    if(codec == nullptr){
        codec = avcodec_find_encoder((AVCodecID)properties_["encoderId"].as_int64());
    }

    if(codec == nullptr)
        return -1;

    codec_context = avcodec_alloc_context3(codec);
    codec_context->thread_count = 0;
    codec_context->pix_fmt = (AVPixelFormat)format.format;
    codec_context->width = format.width;
    codec_context->height = format.height;
    codec_context->framerate = {int(format.fps),1};
    codec_context->time_base = /*{1,30};//*/{format.timebase.numerator,format.timebase.denominator};
    codec_context->bit_rate = properties_["bitrate"].as_double();
    codec_context->rc_max_rate = codec_context->bit_rate * properties_["bitrateMaxScale"].as_double();
    codec_context->rc_min_rate = codec_context->bit_rate * properties_["bitrateMinScale"].as_double();
    codec_context->gop_size = format.fps * properties_["keyframeInterval"].as_double();
    codec_context->max_b_frames = properties_["bframeMax"];
    /* Some formats want stream headers to be separate. */
    const AVOutputFormat* out_foramt = av_guess_format(NULL,properties_["adviceUri"].as_string().c_str(),NULL);
    if(out_foramt){
        if (out_foramt->flags & AVFMT_GLOBALHEADER)
            codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //H264 quality codec param
    codec_context->qmin = properties_["qMin"].as_double();
    codec_context->qmax = properties_["qMax"].as_double();

    if (codec_context->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        codec_context->max_b_frames = 2;
    }
    if (codec_context->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        codec_context->mb_decision = 2;
    }

    if(codec_context->codec_id == AV_CODEC_ID_MPEG1VIDEO)
            codec_context->profile = FF_PROFILE_H264_HIGH;

    //Optional Param
    // Set H264 preset and tune
    AVDictionary *param = 0;
    av_dict_set(&param, "preset", properties_["preset"].as_string().c_str(), 0);
    av_dict_set(&param, "tune", properties_["tune"].as_string().c_str(), 0);

    if (avcodec_open2(codec_context, codec,&param) < 0){
         MR_ERROR("Failed to open encoder (encoder open failed) \n!");
        return -1;
    }
    av_dict_free(&param);

    parameters_ = avcodec_parameters_alloc();
    avcodec_parameters_from_context(parameters_,codec_context);

    Format format_out_;
    format_out_.type = AVMEDIA_TYPE_VIDEO;
    format_out_.codec = codec->id;
    format_out_.format = codec_context->pix_fmt;
    format_out_.width = codec_context->width;
    format_out_.height = codec_context->height;
    format_out_.fps = format.fps;
    format_out_.bitrate = properties_["bitrate"].as_double();
    format_out_.codec_parameters = parameters_;
    format_out_.codec_context = codec_context;

    //NOTE:codec use fps as timebase, so codec_parameters has right data
    //but use higher precise format.timebase as stream's timebase, and processed by receivers
    format_out_.timebase = format.timebase;

    update_pin_format(kInputPin,0,0,format);
    update_pin_format(kOutputPin,0,0,format_out_);
    return 0;
}

int32_t VideoEncoderFFmpegFilter::close_encoder()
{
    quit_encode_ = true;
    encode_condition_.notify_one();
    if(encode_thread_.joinable())
        encode_thread_.join();
    return 0;
}

int32_t VideoEncoderFFmpegFilter::encode_a_frame(FramePointer frame)
{
    int pts = 0;
    if(frame->frame)
        frame->frame->pts;
    //frame->frame->pts /= 33;

    int ret = avcodec_send_frame(codec_context, frame->frame);
    if (ret < 0) {
        return ret;
    }

    while (ret >= 0) {
        AVPacket* encode_packet = av_packet_alloc();
        ret = avcodec_receive_packet(codec_context, encode_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            av_packet_free(&encode_packet);
            break;
        }
        else if (ret < 0) {
            av_packet_free(&encode_packet);
            return ret;
        }

        if(encode_packet->size == 0 || encode_packet->data == nullptr)
            return 0;

        //static int32_t bytes = 0;
        //bytes += encode_packet->size;
        //MR_LOG_DEAULT("pts:{} totle:{} avrg bitrate:{:.2f}kbps frame pts:{}",encode_packet->pts,bytes,bytes*8.0/(pts/1000.0)/1000,frame->frame->pts);
        auto new_frame = Frame::make_packet(encode_packet);
        new_frame->releaser = sdp_frame_free_packet_releaser;
        get_pin(kOutputPin,0)->deliver(new_frame);
    }
    return 0;
}

}


