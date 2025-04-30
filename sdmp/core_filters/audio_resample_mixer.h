#ifndef DATAGRABBER_H
#define DATAGRABBER_H
#include "sdmp_audio_resampler.h"
#include "sdmp_general_filter.h"
#include "sdmp_audio_mixer.h"
namespace mr::sdmp {


COM_MULTITHREADED_OBJECT(
"B52E587E-92D4-40A3-A6F4-808CAE3B835C",
R"({
  "clsid": "B52E587E-92D4-40A3-A6F4-808CAE3B835C",
  "describe": "audio resample/mixer",
  "filtertype": ["audioProcessor"],
  "name": "audioResampleMixer",
  "properties": [
    {
      "name": "format",
      "type": "string",
      "value": "s16",
      "describe":"s16,s32,s64,flt,dbl, av_get_sample_fmt from samplefmt.c"
    },
    {
      "name": "channels",
      "type": "number",
      "value": 2
    },
    {
      "name": "samplerate",
      "type": "number",
      "value": 48000
    },
    {
       "name": "volume",
       "type": "number",
       "value": 1.0
    }
  ],
  "type": "sdp-filter"
})",
AudioResampleMixer)
, public GeneralFilterTypedAs<AudioResampleMixer>
{
public:
    AudioResampleMixer();
    COM_MAP_BEGINE(AudioResampleMixer)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const Value &filter_values) override;
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin) override;
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index) override;
    virtual int32_t receive(IPin *input_pin, FramePointer frame) override;
    virtual int32_t requare(int32_t duration, const std::vector<PinIndex> &output_pins) override;

    virtual int32_t disconnect_input(int32_t input_pin) override;
    // GeneralFilter interface
public:
    virtual int32_t property_changed(const std::string &property, Value &symbol) override;
private:
    void refresh_property();
private:
    int32_t output_fillmode_ = kStretchFill;
    Format  format_out_;
    std::vector<std::shared_ptr<SdpAudioResampler>> resamplers_;
    AudioMixer mixer_;

    std::shared_ptr<uint8_t> pcm_mixer_src_;
    int32_t pcm_mixer_src_size_ = 0;
};

};
#endif // DATAGRABBER_H
