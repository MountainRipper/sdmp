#include "audio_output_participant.h"
#include "sdp_factory_implement.h"
#include "sdpi_factory.h"


namespace mr::sdmp {

COM_REGISTER_OBJECT(AudioOutputParticipantFilter)


AudioOutputParticipanOutputPin::AudioOutputParticipanOutputPin(){

}

int32_t AudioOutputParticipanOutputPin::initialize(FilterPointer filter, const std::vector<Format> &formats, PinDirection direction)
{
    audio_filter_ = static_cast<AudioOutputParticipantFilter*>(filter.Get());
    return GeneralPin::initialize(filter,formats,direction);
}

int32_t AudioOutputParticipanOutputPin::requare_samples(uint8_t *pcm, int32_t samples){
    return audio_filter_->requare_samples(pcm,samples);
}

AudioOutputParticipantFilter::AudioOutputParticipantFilter()
{        

}

AudioOutputParticipantFilter::~AudioOutputParticipantFilter()
{
    MP_LOG_DEAULT("AudioOutputParticipantFilter::~AudioOutputParticipantFilter() {} ", id_.data());
    //disconnect to engine
    quit_flag_ = true;    
}

int32_t AudioOutputParticipantFilter::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_AUDIO,kInputPin);

    FilterPointer filter = FilterPointer(static_cast<IFilter*>(this));
    FilterPinFactory<AudioOutputParticipanOutputPin>::filter_create_pin(filter,AVMEDIA_TYPE_AUDIO,kOutputPin);

    GeneralFilter::initialize(graph,filter_values);

    cache_need_ms_ = properties_["cacheDuration"];
    hunger_need_ms_ = properties_["cacheHungerDuration"];
    output_engine_ = properties_["idEngine"].as_string();
    handler_ = (IFilterHandlerDataGrabber*)(void*)properties_["grabber"];
    return 0;
}

int32_t AudioOutputParticipantFilter::get_property(const std::string &property, Value &value)
{
    if(property == kFilterPropertyCurrentPts){
        value = current_pts_;        
    }
    else
        return GeneralFilter::get_property(property,value);

    return 0;
}

int32_t AudioOutputParticipantFilter::process_command(const std::string &command, const Value& param)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(command == kGraphCommandPlay){
        connect_to_device();
    }
    if(command == kGraphCommandSeek){
        resampler_.clear();
        sender_eos_ = false;
        last_push_pts_ = param;
    } else if(command == kGraphCommandStop) {    
        resampler_.clear();
        last_push_pts_ = 0;
        FilterHelper::disconnect_output(this);
    }
    return GeneralFilter::process_command(command,param);
}

int32_t AudioOutputParticipantFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    const auto& formats = sender_pin->formats();
    int index = -1;
    for (auto& item : formats) {
        ++index;
        if(item.type != AVMEDIA_TYPE_AUDIO)
            continue;
        if(item.samplerate == 0 || item.channels == 0)
            continue;

        format_input_ = item;
        update_pin_format(kInputPin,0,0,format_input_);

        if(handler_){
            handler_->grabber_get_format(id_,&item);
        }

        return index;
    }
    update_pin_format(kInputPin,0,0,sdmp::Format());
    return -1;
}


int32_t AudioOutputParticipantFilter::connect_constraint_output_format(IPin *pin, const std::vector<Format> &format)
{
    if(format.empty())
        return -1;
    format_output_ = format[0];
    resampler_.reset(format_output_.samplerate,format_output_.channels,(AVSampleFormat)format_output_.format);
    update_pin_format(kOutputPin,0,0,format_output_);

    frame_size_ = 1024;
    AVCodecParameters* params = (AVCodecParameters*)format_output_.codec_parameters;
    if(params == nullptr)
        return 0;
    frame_size_ = params->frame_size;
    return 0;
}


int32_t AudioOutputParticipantFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t AudioOutputParticipantFilter::receive(IPin* input_pin,FramePointer frame)
{   
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(status_ == kStatusStoped)
            return 0;
    }

    if(frame->frame == nullptr){
        if(frame->flag & kFrameFlagEos){
            sender_eos_ = true;
        }
        return -1;
    }

    if(frame->frame->format != format_input_.format)
        return -2;
    std::lock_guard<std::mutex> lock(mutex_);
    resampler_.push_audio_samples(frame->frame);
    last_push_pts_ = frame->frame->pts + frame->frame->nb_samples * 1000 / format_input_.samplerate;
    //MP_LOG_DEAULT("audio last_push_pts_:{}",last_push_pts_);
    return 0;
}



int32_t AudioOutputParticipantFilter::requare(int32_t duration,const std::vector<PinIndex>& output_pins)
{
    if(status_ == kStatusEos)
        return kErrorFilterEos;
    std::lock_guard<std::mutex> lock(mutex_);
    int32_t samples_ms = resampler_.samples() * 1000.0 / format_output_.samplerate;
    if(samples_ms < hunger_need_ms_){
        return cache_need_ms_ - samples_ms;
    }
    return 0;
}

int32_t AudioOutputParticipantFilter::disconnect_output(int32_t output_pin, IPin *input_pin)
{
    //ignore disconnect call ,keep connect to engine
    if(!quit_flag_)
        return 0;
    return GeneralFilter::disconnect_output(output_pin,input_pin);
}

int32_t AudioOutputParticipantFilter::property_changed(const std::string& name,Value& symbol)
{
    if(name == "cacheDuration"){
        cache_need_ms_ = properties_["cacheDuration"];
    }
    else if(name == "cacheHungerDuration"){
        hunger_need_ms_ = properties_["cacheHungerDuration"];
    }
    else if(name == "channelMapping") {
        channel_mapping_.clear();
        const std::vector<double>& number_vector = symbol;
        if(number_vector.size())
            PcmConvertor::valid_channel_mapping(number_vector,channel_mapping_,format_output_.channels);
    }
    else if(name == "grabber"){
        void* ptr = properties_["grabber"];
        handler_ = (IFilterHandlerDataGrabber*)ptr;
    }
    return 0;
}


int32_t AudioOutputParticipantFilter::requare_samples(uint8_t *pcm, int32_t samples)
{
    if(status_ != kStatusRunning)
        return 0;

    std::lock_guard<std::mutex> lock(mutex_);

    double volume_percent = properties_["volume"];
    int32_t volume = volume_percent * 100;
    int32_t samples_has = resampler_.samples();
    //MP_LOG_DEAULT("### audio output cur:{}  samples:{}",last_push_pts_,samples_has);

    if(samples_has >= samples){        
        current_pts_ = last_push_pts_ - samples_has * 1000.0 / format_output_.samplerate;
        set_timeline(current_pts_);

        const auto& sample = resampler_.pull(samples);
        if(sample == nullptr){
            return 0;
        }
        auto audio = sample->frame;
        if(channel_mapping_.empty()){
            //direct copy to device
            memcpy(pcm,audio->data[0],audio->linesize[0]);
        }
        else{
            //remapp and copy
            if(format_output_.format == AV_SAMPLE_FMT_S16)
                PcmConvertor::remapping_packeted<int16_t>(audio->data[0],pcm,samples,format_output_.channels,channel_mapping_);
            else if(format_output_.format == AV_SAMPLE_FMT_S32)
                PcmConvertor::remapping_packeted<int32_t>(audio->data[0],pcm,samples,format_output_.channels,channel_mapping_);
            else if(format_output_.format == AV_SAMPLE_FMT_S64)
                PcmConvertor::remapping_packeted<int64_t>(audio->data[0],pcm,samples,format_output_.channels,channel_mapping_);
            else if(format_output_.format == AV_SAMPLE_FMT_FLT)
                PcmConvertor::remapping_packeted<float>(audio->data[0],pcm,samples,format_output_.channels,channel_mapping_);
            else if(format_output_.format == AV_SAMPLE_FMT_DBL)
                PcmConvertor::remapping_packeted<double>(audio->data[0],pcm,samples,format_output_.channels,channel_mapping_);
            else if(format_output_.format == AV_SAMPLE_FMT_U8)
                PcmConvertor::remapping_packeted<uint8_t>(audio->data[0],pcm,samples,format_output_.channels,channel_mapping_);
        }

        if(handler_){
            AVFrame* frame = av_frame_alloc();
            frame->sample_rate    = format_output_.samplerate;
            frame->format         = format_output_.format;
            frame->nb_samples     = samples;
            av_channel_layout_default(&frame->ch_layout,format_output_.channels);
            av_frame_get_buffer(frame,1);
            memcpy(frame->data[0],pcm,audio->linesize[0]);

            std::shared_ptr<sdmp::Frame> sdp_frame = sdmp::Frame::make_frame(frame);
            sdp_frame->releaser = sdp_frame_free_frame_releaser;
            handler_->grabber_get_frame(id_,sdp_frame);
        }
        return volume;
    }
    else if(sender_eos_){
        if(samples_has > 0){
            const auto& sample = resampler_.pull(samples_has);
            if(sample == nullptr){
                return 0;
            }
            auto audio = sample->frame;
            memcpy(pcm,audio->data[0],audio->linesize[0]);
        }

        switch_status(kStatusEos);
        current_pts_ += samples * 1000.0 / format_output_.samplerate;
        set_timeline(current_pts_);
        set_timeline_status(kTimelineOver);
    }
    else{
        set_timeline_status(kTimelineBlocking);
        MP_LOG_DEAULT(">>>>Audio Buffer Hungry");
    }
    return volume;
}

int32_t AudioOutputParticipantFilter::connect_to_device()
{
    if(format_output_.samplerate > 0)
        return 0;//already connected

    if(output_engine_.empty())
        return -1;

    auto device_filter = Factory::factory()->engine()->get_audio_output_filter(output_engine_);
    if(!device_filter){
        MP_LOG_DEAULT("{} can't find march output engine {}",__FUNCTION__,output_engine_);
        return  -2;
    }
    graph_->do_connect(this,device_filter.Get(),0,0);
    std::vector<double> vec = properties_["channelMapping"];
    if(vec.size())
        PcmConvertor::valid_channel_mapping(vec,channel_mapping_,format_output_.channels);
    return 0;
}


}
