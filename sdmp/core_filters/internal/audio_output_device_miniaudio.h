#ifndef AUDIOOUTPUTDEVICEMINIAUDIO_H
#define AUDIOOUTPUTDEVICEMINIAUDIO_H
#include "miniaudio.h"
#include "core_includes.h"

namespace sdp {

COM_MULTITHREADED_OBJECT(
"cbdb4bb8-b881-11eb-a370-7b276191dfef",
R"({
  "clsid": "cbdb4bb8-b881-11eb-a370-7b276191dfef",
  "describe": "audio playback device filter powered by miniaudio",
  "filtertype": ["AudioOutput"],
  "name": "miniaudioOutput",
  "properties": [],
  "type": "sdp-filter"
})",
AudioOutputDeviceMiniaudioFilter)
, public GeneralFilterBase
{
public:
    AudioOutputDeviceMiniaudioFilter();
    virtual ~AudioOutputDeviceMiniaudioFilter();

    COM_MAP_BEGINE(AudioOutputDeviceMiniaudioFilter)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const sol::table &config);
    virtual int32_t get_property(const std::string& property,NativeValue& value);
    virtual int32_t process_command(const std::string &command, const NativeValue& param);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin* input_pin,FramePointer frame);
    virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins);
    virtual int32_t master_loop(bool before_after);

    //COM object support
    virtual int32_t FinalRelease();
    int32_t on_playback(void* pcm,int32_t frames);
private:
    int32_t pin_requare_data(void* pcm,int32_t frames);
private:
    std::shared_ptr<ma_context> context_;
    std::shared_ptr<ma_device> device_;
    std::shared_ptr<BufferObject> device_id_;

    PinPointer  pin_;
    ma_pcm_rb   pull_ring_buffer_;

    int32_t     samplerate_ = 0;
    int32_t     channels_   = 0;
    ma_format   format_     = ma_format_s16;
    int32_t     frame_size_ = 0;
    AVCodecParameters av_params_;
};


}

#endif // AUDIOOUTPUTSDL_H
