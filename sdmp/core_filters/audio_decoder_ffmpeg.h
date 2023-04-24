#ifndef AUDIODECODERFFMPEG_H
#define AUDIODECODERFFMPEG_H
#include "sdmp_general_filter.h"

namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"98c87260-a203-11ed-9bd9-bfba9c9e5f53",
R"({
    "clsid": "98c87260-a203-11ed-9bd9-bfba9c9e5f53",
    "describe": "audio decoder powered by ffmpeg",
    "filtertype": ["AudioDecoder"],
    "name": "audioDecoderFFmpeg",
    "properties": [],
    "type": "sdp-filter"
}
)",
AudioDecoderFFmpegFilter)
, public GeneralFilterTypedAs<AudioDecoderFFmpegFilter>
{
public:    
    AudioDecoderFFmpegFilter();
    ~AudioDecoderFFmpegFilter();

    COM_MAP_BEGINE(AudioDecoderFFmpegFilter)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const Value &filter_values);
    virtual int32_t process_command(const std::string &command, const Value& param);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin* input_pin,FramePointer frame);
    virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins);

    // FilterGeneral interface
public:
    virtual int32_t property_changed(const std::string& name,Value& symbol);
private:
    int32_t close_decoder();
    int32_t open_decoder(const Format& format);
private:
    AVCodecContext* decoder_ = nullptr;
    Format          format_out_;
    SwrContext*     swr_to_packeted_ = nullptr;

    std::mutex      decoder_mutex_;
};

}


#endif // MEDIASOURCEFILTER_H
