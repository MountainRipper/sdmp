#ifndef AUDIO_MIXER_INPUT_PIN_H
#define AUDIO_MIXER_INPUT_PIN_H
#include <mutex>
#include "sdp_general_pin.h"
#include "sdpi_objects.h"
#include "sdpi_filter_extentions.h"
namespace mr::sdmp {

COM_MULTITHREADED_OBJECT("0ef73af8-a212-11ed-b92f-775b7675e60a", "", AudioMixerInputPin)
                         , public GeneralPin
{
public:
    AudioMixerInputPin();

    COM_MAP_BEGINE(AudioMixerInputPin)
        COM_INTERFACE_ENTRY(IPin)
    COM_MAP_END()

    // IPin interface
public:
    virtual int32_t initialize(FilterPointer filter, const std::vector<Format>& formats, PinDirection direction);
    virtual int32_t connect(IPin *pin);
    virtual int32_t disconnect(IPin *pin);
    virtual int32_t requare(void *pcm, int32_t frames);
    virtual int32_t receive(FramePointer frame);
private:
    std::set<IAudioOutputParticipantPin*> sender_pins_;
    std::mutex sender_mutex_;
    int32_t channels_ = 0;
    int32_t format_ = 0;
    int32_t sample_bytes_chs_ = 0;
    int32_t sdl_format_ = 0;

    uint8_t* pcm_mixer_dest_ = nullptr;

    std::shared_ptr<uint8_t> pcm_mixer_src_;
    int32_t pcm_mixer_src_size_ = 0;
};

}

#endif // AUDIO_MIXER_INPUT_PIN_H
