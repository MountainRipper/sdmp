#include "audio_mixer_input_pin.h"
#include "core_includes.h"
extern "C"
{
#include "sdl_mixer/SDL_mixer.h"
}

namespace sdp {

AudioMixerInputPin::AudioMixerInputPin()
{

}


int32_t AudioMixerInputPin::initialize(FilterPointer filter, const std::vector<Format> &formats, PinDirection direction)
{
    (void)direction;
    if(formats.empty())
        return kErrorInvalidParameter;

    GeneralPin::initialize(filter,formats,kInputPin);

    auto& format = formats[0];
    channels_ = format.channels;
    format_ = format.format;
    sample_bytes_chs_ = channels_ * av_get_bytes_per_sample((AVSampleFormat)format_);

    sdl_format_ = 0;
    if(format_ == AV_SAMPLE_FMT_S16)
        sdl_format_ = AUDIO_S16;
    if(format_ == AV_SAMPLE_FMT_FLT)
        sdl_format_ = AUDIO_F32;
    if(format_ == AV_SAMPLE_FMT_S32)
        sdl_format_ = AUDIO_S32;

    return 0;
}

int32_t AudioMixerInputPin::connect(IPin *pin)
{
    if(pin == nullptr)
        return -1;

    if(pin->direction() != kOutputPin)
        return -2;

    ComPointer<IAudioOutputParticipantPin> audio_pin = ComPointer<IPin>(pin).As<IAudioOutputParticipantPin>();
    if(audio_pin)
    {
        std::lock_guard<std::mutex> lock(sender_mutex_);
        sender_pins_.insert(audio_pin.Get());
    }

    if(sender_pins_.size())
    {
        filter_->process_command(kGraphCommandPlay,.0);
    }
    return 0;
}

int32_t AudioMixerInputPin::disconnect(IPin *pin)
{
    if(pin){
        ComPointer<IAudioOutputParticipantPin> audio_pin = ComPointer<IPin>(pin).As<IAudioOutputParticipantPin>();
        std::lock_guard<std::mutex> lock(sender_mutex_);
        sender_pins_.erase(audio_pin.Get());
    }

    if(sender_pins_.empty())
    {
        filter_->process_command(kGraphCommandStop,.0);
    }
    return 0;
}

int32_t AudioMixerInputPin::requare(void *pcm, int32_t frames)
{
    int32_t bytes = frames * sample_bytes_chs_;
    memset(pcm,0,bytes);

    if(sdl_format_ == 0)
        return 0;

    pcm_mixer_dest_ = (uint8_t*)pcm;

    if(pcm_mixer_src_size_ < bytes){
        pcm_mixer_src_size_ = bytes + 128;
        pcm_mixer_src_ = sdp::BufferUtils::create_shared_buffer(pcm_mixer_src_size_);
    }
    std::lock_guard<std::mutex> lock(sender_mutex_);
    for(auto pin : sender_pins_){
        memset(pcm_mixer_src_.get(),0,pcm_mixer_src_size_);
        int32_t volume = pin->requare_samples(pcm_mixer_src_.get(),frames);
        int sdl_mix_volume = volume/100.0*SDL_MIX_MAXVOLUME;
        SDL_MixAudioFormat(pcm_mixer_dest_,pcm_mixer_src_.get(),sdl_format_,bytes,sdl_mix_volume);
    }
    return 0;
}

int32_t sdp::AudioMixerInputPin::receive(FramePointer frame)
{
//    if(frame == nullptr)
//        return -1;
//    if(frame->frame == nullptr)
//        return -2;
//    uint8_t* src = frame->frame->data[0];
//    int line_size = frame->frame->linesize[0];
//    if(src == nullptr || line_size == 0)
//        return -3;

//    SDL_MixAudioFormat(pcm_mixer_dest_,src,sdl_format_,line_size,SDL_MIX_MAXVOLUME);
    return 0;
}

}



