#ifndef AUDIOOUTPUTENGINE_H
#define AUDIOOUTPUTENGINE_H
#include <mutex>
#include "sdp_general_filter.h"
#include "sdp_general_pin.h"
#include "sdpi_filter_extentions.h"
#include "sdp_timeline_support.h"
#include "sdp_audio_resampler.h"

namespace mr::sdmp {

class AudioOutputParticipantFilter;

COM_MULTITHREADED_OBJECT("410ed500-a208-11ed-a4e0-47541aa44ff4","",AudioOutputParticipanOutputPin )
, public GeneralPin
, public IAudioOutputParticipantPin
{
public:
    COM_MAP_BEGINE(AudioOutputParticipanOutputPin)
        COM_INTERFACE_ENTRY(IPin)
        COM_INTERFACE_ENTRY(IAudioOutputParticipantPin)
    COM_MAP_END()

    AudioOutputParticipanOutputPin();

    virtual int32_t initialize(FilterPointer filter, const std::vector<Format>& formats, PinDirection direction);
    virtual int32_t requare_samples(uint8_t* pcm,int32_t samples);
private:
    AudioOutputParticipantFilter* audio_filter_ = nullptr;
};

COM_MULTITHREADED_OBJECT(
"fde579e0-a208-11ed-b4d0-9bd271f94509",
R"({
    "clsid": "fde579e0-a208-11ed-b4d0-9bd271f94509",
    "describe": "audio output participants for internal audio engine",
    "filtertype": ["AudioOutput"],
    "name": "audioOutputParticipant",
    "properties": [
     {
       "name": "cacheDuration",
       "type": "number",
       "value": 500.0
     },
     {
       "name": "cacheHungerDuration",
       "type": "number",
       "value": 300.0
     },
     {
       "name": "channelMapping",
       "type": "number_array",
       "value": []
     },
     {
       "name": "grabber",
       "type": "pointer",
       "value": 0
     },
     {
       "name": "volume",
       "type": "number",
       "value": 1.0
     }
    ],
    "type": "sdp-filter"
})",
AudioOutputParticipantFilter )
, public GeneralFilterTypedAs<AudioOutputParticipantFilter>
, public sdmp::GeneralTimeline
{
public:
    AudioOutputParticipantFilter();
    ~AudioOutputParticipantFilter();

    COM_MAP_BEGINE(AudioOutputParticipantFilter)
        COM_INTERFACE_ENTRY(IFilter)
        COM_INTERFACE_ENTRY(IFilterExtentionTimeline)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const Value &config_value);
    virtual int32_t get_property(const std::string &property, Value &value);
    virtual int32_t process_command(const std::string &command, const Value& param);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_constraint_output_format(IPin *output_pin, const std::vector<Format> &format);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin* input_pin,FramePointer frame);
    virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins);

    virtual int32_t disconnect_output(int32_t output_pin,IPin* input_pin);
    // FilterGeneral interface
public:
    virtual int32_t property_changed(const std::string& name,Value& symbol);
    // IFilterExtentionAudioOutputParticipant interface
public:
    virtual int32_t requare_samples(uint8_t *pcm, int32_t samples);

private:
    virtual int32_t connect_to_device();
private:
    Format                      format_input_;
    Format                      format_output_;
    int32_t                     frame_size_             = 0;
    int32_t                     cache_need_ms_          = 0;
    int32_t                     hunger_need_ms_         = 0;
    std::string                 output_engine_;
    SdpAudioResampler              resampler_;
    std::mutex                  mutex_;
    IFilterHandlerDataGrabber*  handler_                = nullptr;
    std::vector<int32_t>        channel_mapping_;


    int64_t                     last_push_pts_          = 0;
    int64_t                     current_pts_            = 0;
    bool                        sender_eos_             = false;
    bool                        quit_flag_              = false;
};

}

#endif // MEDIASOURCEFILTER_H
