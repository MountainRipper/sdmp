#ifndef AUDIOENCODERFFMPEG_H
#define AUDIOENCODERFFMPEG_H

#include "sdp_general_filter.h"
#include "sdp_audio_resampler.h"
namespace sdp {

COM_MULTITHREADED_OBJECT(
"5b092074-b87f-11eb-addf-b73fe5092442",
R"({
  "clsid": "5b092074-b87f-11eb-addf-b73fe5092442",
  "describe": "audio encoder powered by ffmpeg",
  "filtertype": ["AudioEncoder"],
  "name": "audioEncoderFFmpeg",
  "properties": [
    {
      "name": "adviceUri",
      "type": "string",
      "value": ""
    },
    {
      "name": "bitrate",
      "type": "number",
      "value": 128000.0
    },
    {
      "name": "encoderId",
      "type": "number",
      "value": 86018.0
    },
    {
      "name": "encoderName",
      "type": "string",
      "value": "aac"
    }
  ],
  "type": "sdp-filter"
})",
AudioEncoderFFmpegFilter)
, public GeneralFilterObjectRoot<AudioEncoderFFmpegFilter>
{    
public:
    AudioEncoderFFmpegFilter();

    COM_MAP_BEGINE(AudioEncoderFFmpegFilter)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const sol::table &config);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin *input_pin, FramePointer frame);
    virtual int32_t requare(int32_t duration, const std::vector<PinIndex> &output_pins);
    virtual int32_t process_command(const std::string &command, const NativeValue &param);

private:
    void    encode_proc();
    int32_t open_encoder(const Format &format);
    int32_t close_encoder();
    int32_t encode_a_frame(AVFrame* frame);
private:
    AVCodecContext *codec_context = nullptr;
    AVCodecParameters* parameters_ = nullptr;
    //FrameCache  frames_;
    SdpAudioResampler resampler_;
//    std::thread     encode_thread_;
//    std::mutex      encode_mutex_;
//    std::condition_variable encode_condition_;
//    bool            quit_encode_    = false;
//    StatusWait      pause_wait_;
};

}

#endif // VIDEOENCODERFFMPEG_H
