#ifndef AUDIO_RESAMPLE_POOL_H_
#define AUDIO_RESAMPLE_POOL_H_
#include "core_includes.h"
#include "avcpp/audioresampler.h"

namespace sdp {

class SdpAudioResampler
{
public:
    SdpAudioResampler(void);
    ~SdpAudioResampler(void);
public:
    int32_t reset(int32_t samplerate,int32_t channels,AVSampleFormat format);
    int32_t set_channel_mapping(const std::vector<int32_t>& mapping);
    int32_t push_audio_samples(const av::AudioSamples& sample);
    int32_t push_audio_samples(int32_t samplerate, int32_t channels, AVSampleFormat format, int32_t samples, uint8_t *pcm[]);

    av::AudioSamples pull(int32_t samples);

    int32_t samples();
    int32_t samplerate();
    int32_t channles();
    AVSampleFormat format();
    int32_t sample_bytes();

    int32_t clear();
private:
    int32_t         samplerate_             = 0;
    int32_t         channels_               = 0;
    AVSampleFormat  format_                 = AV_SAMPLE_FMT_NONE;

    int32_t         samplerate_in_          = 0;
    int32_t         channels_in_            = 0;
    AVSampleFormat  format_in_              = AV_SAMPLE_FMT_NONE;

    int32_t         sample_bytes_           = 0;
    std::vector<int32_t> mapping_request_;
    int             mapping_index_[16]      = {0};
    bool            need_create_resample_   = true;


    av::AudioResampler  resampler_;
};
}


#endif
