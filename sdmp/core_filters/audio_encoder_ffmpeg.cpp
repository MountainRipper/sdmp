#include "audio_encoder_ffmpeg.h"


namespace mr::sdmp{


COM_REGISTER_OBJECT(AudioEncoderFFmpegFilter)

AudioEncoderFFmpegFilter::AudioEncoderFFmpegFilter()
{
}

int32_t AudioEncoderFFmpegFilter::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_AUDIO,kInputPin);
    create_general_pin(AVMEDIA_TYPE_AUDIO,kOutputPin);
    return GeneralFilter::initialize(graph,filter_values);
}

int32_t sdmp::AudioEncoderFFmpegFilter::process_command(const std::string &command, const Value &param)
{
    GeneralFilter::process_command(command,param);
    if(command == kGraphCommandPause){

    }
    else if(command == kGraphCommandStop){

    }
    else if(command == kGraphCommandPlay){
    }
    return 0;
}

int32_t sdmp::AudioEncoderFFmpegFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    const auto& formats = sender_pin->formats();
    int index = -1;
    for(auto& item : formats){
        ++index;
        int32_t ret = open_encoder(item);
        if(ret < 0)
            continue;      
        return index;
    }
    close_encoder();
    return -1;
}

int32_t sdmp::AudioEncoderFFmpegFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t sdmp::AudioEncoderFFmpegFilter::receive(IPin *input_pin, FramePointer frame)
{
    auto av_frame = frame->frame;
    // resampler_.push_audio_samples(av_frame->sample_rate,av_frame->ch_layout.nb_channels,
    //                       (AVSampleFormat)av_frame->format,av_frame->nb_samples,av_frame->data);

    resampler_.push_audio_samples(av_frame);
    while(resampler_.samples() >= codec_context->frame_size){
        auto audio = resampler_.pull(codec_context->frame_size);
        encode_a_frame(audio->frame);
    }
    return 0;
}

int32_t sdmp::AudioEncoderFFmpegFilter::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{    
    return duration;
}

int32_t AudioEncoderFFmpegFilter::open_encoder(const Format& format)
{
    close_encoder();

    const AVCodec* codec = nullptr;
    std::string encoderName = properties_["encoderName"];
    if(!encoderName.empty()){
        codec = avcodec_find_encoder_by_name(encoderName.c_str());
    }

    if(codec == nullptr){
        int64_t value = properties_["encoderId"];
        codec = avcodec_find_encoder((AVCodecID)value);
    }

    if(codec == nullptr)
        return -1;

    codec_context = avcodec_alloc_context3(codec);
    ff_set_context_channels(codec_context,format.channels);
    codec_context->sample_rate = format.samplerate;
    codec_context->time_base.num = 1;
    codec_context->time_base.den = format.samplerate;
    codec_context->bit_rate = properties_["bitrate"];
    // codec_context->frame_size = 480;

    int max_bytes = 0;
    const enum AVSampleFormat *support_format = codec->sample_fmts;
    while (*support_format != AV_SAMPLE_FMT_NONE) {
        auto bytes = av_get_bytes_per_sample(*support_format);
        if (bytes > max_bytes)
        {
            //get best bit-depth format
            max_bytes = bytes;
            codec_context->sample_fmt = *support_format;
        }
        else if(bytes == max_bytes && !av_sample_fmt_is_planar(*support_format)){
            //usr packeted pcm first
            codec_context->sample_fmt = *support_format;
        }
        support_format++;
    }
    /* Some formats want stream headers to be separate. */
    std::string advice_uri = properties_["adviceUri"];
    const AVOutputFormat* out_foramt = av_guess_format(NULL,advice_uri.c_str(),NULL);
    if(out_foramt){
        if (out_foramt->flags & AVFMT_GLOBALHEADER)
            codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(codec_context, codec,nullptr) < 0){
         MR_ERROR( "Failed to open encoder (encoder open failed)!");
        return -1;
    }

    parameters_ = avcodec_parameters_alloc();
    avcodec_parameters_from_context(parameters_,codec_context);

    Format format_out_;
    format_out_.type = AVMEDIA_TYPE_AUDIO;
    format_out_.codec = codec->id;
    format_out_.format = codec_context->sample_fmt;
    format_out_.samplerate = codec_context->sample_rate;
    format_out_.channels = codec_context->ch_layout.nb_channels;
    format_out_.bitrate = int64_t(properties_["bitrate"]);
    format_out_.codec_parameters = parameters_;
    format_out_.codec_context = codec_context;

    format_out_.timebase = format.timebase;

    sync_update_pin_format(kInputPin,0,0,format);
    sync_update_pin_format(kOutputPin,0,0,format_out_);

    resampler_.reset(format.samplerate,format.channels,(AVSampleFormat)codec_context->sample_fmt);
    sample_bytes_ = av_get_bytes_per_sample((AVSampleFormat)codec_context->sample_fmt);
    return 0;
}

int32_t AudioEncoderFFmpegFilter::close_encoder()
{
    if(codec_context){
        avcodec_free_context(&codec_context);
        codec_context = nullptr;
    }
    if(parameters_){
        avcodec_parameters_free(&parameters_);
        parameters_ = nullptr;
    }
    return 0;
}

int32_t AudioEncoderFFmpegFilter::encode_a_frame(AVFrame *frame)
{
    //add 1 to avoid div by zero
    samples_totle_ += (frame->linesize[0] / sample_bytes_);
    frame->time_base = {1,frame->sample_rate};
    frame->pts = samples_totle_;

    int ret = avcodec_send_frame(codec_context, frame);
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

        // if(encode_packet->pts < 0)
        //     continue;
        auto new_frame = Frame::make_packet(encode_packet);
        new_frame->releaser = sdmp_frame_free_packet_releaser;
        get_pin(kOutputPin,0)->deliver(new_frame);
    }
    return 0;
}

}


