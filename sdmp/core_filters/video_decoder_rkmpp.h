#ifndef VIDEODECODERRKMPP_H
#define VIDEODECODERRKMPP_H
#include "sdp_general_filter.h"
#if defined(HAS_ROCKCHIP_MPP)
typedef void* MppCtx;
typedef void* MppParam;
typedef void* MppFrame;
namespace sdp {

COM_MULTITHREADED_OBJECT(
"70871d5e-a20f-11ed-8bd3-03062744393b",
R"({
  "clsid": "70871d5e-a20f-11ed-8bd3-03062744393b",
  "describe": "video decoder powered by rockchip mpp",
  "filtertype": ["VideoDecoder"],
  "name": "videoDecoderRkmpp",
  "properties": [],
  "type": "sdp-filter"
})",
VideoDecoderRkmppFilter)
 , public GeneralFilterObjectRoot<VideoDecoderRkmppFilter>
{
public:
    VideoDecoderRkmppFilter();
    ~VideoDecoderRkmppFilter();

    COM_MAP_BEGINE(VideoDecoderRkmppFilter)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const sol::table &config);
    virtual int32_t process_command(const std::string &command, const NativeValue& param);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin* input_pin,FramePointer frame);
    virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins);

    // FilterGeneral interface
public:
    virtual int32_t property_changed(const std::string& name,NativeValue& symbol);
private:
    int32_t open_decoder(const Format &params);
    int32_t decode_a_frame(FramePointer sdp_frame);
    int32_t decode_param();
    int32_t export_frame(MppFrame *mppframe_pointer);
    int32_t close_decoder();
private:
    AVBufferRef         *decoder_ref = nullptr;
    AVCodecParameters   *params_;
    Format              format_out_;
    bool                first_frame_ = true;
    std::shared_ptr<BitStreamConvert> bitstream_converter_;
};

}
#endif //HAS_ROCKCHIP_MPP
#endif // MEDIASOURCEFILTER_H
