#include "audio_decoder_ffmpeg.h"
namespace sdp {

COM_REGISTER_OBJECT(AudioDecoderFFmpegFilter)

AudioDecoderFFmpegFilter::AudioDecoderFFmpegFilter()
{

}

AudioDecoderFFmpegFilter::~AudioDecoderFFmpegFilter()
{
    MP_LOG_DEAULT("AudioDecoderFFmpegFilter::~AudioDecoderFFmpegFilter() {} ", id_.data());
    close_decoder();
}

int32_t AudioDecoderFFmpegFilter::initialize(IGraph *graph, const sol::table &config)
{
    create_general_pin(AVMEDIA_TYPE_AUDIO,kInputPin);
    create_general_pin(AVMEDIA_TYPE_AUDIO,kOutputPin);
    return GeneralFilter::initialize(graph,config);
}

int32_t AudioDecoderFFmpegFilter::process_command(const std::string &command, const NativeValue& param)
{
    if(command == kGraphCommandSeek || command == kGraphCommandStop){
        std::lock_guard<std::mutex> lock(decoder_mutex_);
        if(decoder_)
            avcodec_flush_buffers(decoder_);
    }
    return GeneralFilter::process_command(command,param);
}

int32_t AudioDecoderFFmpegFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    const auto& formats = sender_pin->formats();
    int index = -1;
    for(auto& item : formats){
        ++index;       
        int32_t ret = open_decoder(item);
        if(ret < 0)
            continue;
        return index;
    }
    close_decoder();
    return -1;
}

int32_t AudioDecoderFFmpegFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t AudioDecoderFFmpegFilter::receive(IPin* input_pin,FramePointer frame)
{
    if(status_ == kStatusEos)
        return kErrorFilterEos;
    if(decoder_ == nullptr)
        return kErrorFilterInvalid;

    std::lock_guard<std::mutex> lock(decoder_mutex_);

    int ret = avcodec_send_packet(decoder_,frame->packet);
    if(ret < 0)
        return kErrorFilterDecode;

    AVFrame* pcm_frame = av_frame_alloc();

    do{
        ret = avcodec_receive_frame(decoder_,pcm_frame);
        if (ret < 0 && ret != AVERROR(EAGAIN)){
            break;
        }
        if (ret >= 0){

            AVFrame* packetd_frame = av_frame_alloc();
            packetd_frame->sample_rate = format_out_.samplerate;
            packetd_frame->channels = format_out_.channels;
            packetd_frame->format = (AVSampleFormat)format_out_.format;
            packetd_frame->pts = pcm_frame->pts;
            av_samples_alloc(packetd_frame->data,packetd_frame->linesize,
                             pcm_frame->channels,pcm_frame->nb_samples,(AVSampleFormat)packetd_frame->format,1);

            packetd_frame->nb_samples = swr_convert(swr_to_packeted_,
                        packetd_frame->data,pcm_frame->nb_samples,
                        (const uint8_t**)(pcm_frame->data),pcm_frame->nb_samples);

            auto new_frame = Frame::make_frame(packetd_frame);
            new_frame->releaser = [](AVFrame* frame,AVPacket*){
                av_freep(&frame->data[0]);//release for av_samples_alloc
                av_frame_free(&frame);//release for frame itself
            };           
            get_pin(kOutputPin,0)->deliver(new_frame);
        }
    }while (false);

    av_frame_free(&pcm_frame);

    if(frame->packet == nullptr){
        if(frame->flag & kFrameFlagEos){
            switch_status(kStatusEos);
            deliver_eos_frame();
        }
    }

    return ret;
}



int32_t AudioDecoderFFmpegFilter::requare(int32_t duration,const std::vector<PinIndex>& output_pins)
{
    //decoder no cache ,request sender
    return duration;
}

int32_t AudioDecoderFFmpegFilter::property_changed(const std::string& name,NativeValue& symbol)
{
    return 0;
}

int32_t AudioDecoderFFmpegFilter::close_decoder()
{
    if(decoder_){
        avcodec_free_context(&decoder_);
        decoder_ = nullptr;
    }
    if(swr_to_packeted_){
        swr_free(&swr_to_packeted_);
        swr_to_packeted_ = nullptr;
    }
    update_pin_format(kInputPin,0,0,sdp::Format());
    update_pin_format(kOutputPin,0,0,sdp::Format());
    return 0;
}

int32_t AudioDecoderFFmpegFilter::open_decoder(const Format &format)
{
    close_decoder();

    if(format.type != AVMEDIA_TYPE_AUDIO)
        return -1;
    if(format.codec_parameters == nullptr)
        return -2;
    if(format.samplerate == 0 || format.channels == 0)
        return -3;

    AVCodecID codec_id = (AVCodecID)format.codec;
    const AVCodec* codec = avcodec_find_decoder(codec_id);
    if(codec == nullptr)
        return -4;
    decoder_ = avcodec_alloc_context3(codec);
    if(decoder_ == nullptr)
        return -5;

    AVCodecParameters* params = (AVCodecParameters*)format.codec_parameters;
    if(params)
        avcodec_parameters_to_context(decoder_,params);

    int ret = avcodec_open2(decoder_,codec,nullptr);
    if(ret < 0)
        return ret;


    AVSampleFormat packeted_format = av_get_packed_sample_fmt(decoder_->sample_fmt);
    AVCodecID pcm_codec = (AVCodecID)FormatUtils::sample_format_to_codec_id(packeted_format);


    format_out_.type = AVMEDIA_TYPE_AUDIO;
    format_out_.codec = pcm_codec;
    format_out_.format = packeted_format;
    format_out_.channels = decoder_->channels;
    format_out_.samplerate = decoder_->sample_rate;
    format_out_.timebase = {1,decoder_->sample_rate};

    int bytes_per_second = 0;
    av_samples_get_buffer_size(&bytes_per_second,format_out_.channels,format_out_.samplerate,packeted_format,1);
    format_out_.bitrate = bytes_per_second*8;

    swr_to_packeted_ = swr_alloc_set_opts(swr_to_packeted_,
                       av_get_default_channel_layout(format_out_.channels),packeted_format,format_out_.samplerate,
                       av_get_default_channel_layout(decoder_->channels),decoder_->sample_fmt,decoder_->sample_rate,
                       0,nullptr);

    ret = swr_init(swr_to_packeted_);
    if(ret < 0)
        return ret;

    update_pin_format(kInputPin,0,0,format);
    update_pin_format(kOutputPin,0,0,format_out_);

    return 0;
}

}




