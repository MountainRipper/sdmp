#ifndef AUDIO_RESAMPLE_POOL_H_
#define AUDIO_RESAMPLE_POOL_H_
#include "core_includes.h"

namespace sdp {

class SdpAudioResampler
{
public:
    SdpAudioResampler(void);
    ~SdpAudioResampler(void);
public:
    int32_t reset(int32_t samplerate,int32_t channels,AVSampleFormat format);
    int32_t push_audio_samples(const AVFrame *sample);
    int32_t push_audio_samples(int32_t samplerate, int32_t channels, AVSampleFormat format, int32_t samples, uint8_t *pcm[]);

    FramePointer pull(int32_t samples);

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
    AVChannelLayout channel_layout_;

    int32_t         samplerate_in_          = 0;
    int32_t         channels_in_            = 0;
    AVSampleFormat  format_in_              = AV_SAMPLE_FMT_NONE;
    AVChannelLayout channel_layout_in_;

    int32_t         sample_bytes_           = 0;
    bool            need_create_resample_   = true;

    SwrContext      *resampler_ = nullptr;
};
}


#endif
