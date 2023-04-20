#include <sstream>
#include <iomanip>
#include <spdlog/fmt/bin_to_hex.h>
#include <libyuv.h>
#include <sol/sol.hpp>
#include "core_includes.h"
#include "sdpi_utils.h"
extern "C"{
#include <libavcodec/bsf.h>
}
namespace mr::sdmp {

using namespace std;

struct FackerForNullTypeinfo{

};

const std::type_info &Value::typeid_of_type(ValueType type)
{
    switch(type){
    case kPorpertyNumber      : return typeid(double);
    case kPorpertyString      : return typeid(std::string);
    case kPorpertyBool        : return typeid(bool);
    case kPorpertyNumberArray : return typeid(std::vector<double>);
    case kPorpertyStringArray : return typeid(std::vector<std::string>);
    case kPorpertyPointer     : return typeid(void*);
    case kPorpertyLuaFunction : return typeid(sol::function);
    case kPorpertyLuaTable    : return typeid(sol::table);
    default: return typeid(FackerForNullTypeinfo);
    }
}
ValueType Value::type_of_typeid(const std::type_info & typeinfo){
    if(typeinfo == typeid(double))
        return kPorpertyNumber;
    else if(typeinfo == typeid(std::string))
        return  kPorpertyString;
    else if(typeinfo == typeid(bool))
        return  kPorpertyBool;
    else if(typeinfo == typeid(std::vector<double>))
        return  kPorpertyNumberArray;
    else if(typeinfo == typeid(std::vector<std::string>))
        return  kPorpertyStringArray;
    else if(typeinfo == typeid(void*))
        return  kPorpertyPointer;
    else if(typeinfo == typeid(sol::function))
        return  kPorpertyLuaFunction;
    else if(typeinfo == typeid(sol::table))
        return kPorpertyLuaTable;
    else if(typeinfo == typeid(sol::lua_value) ||
            typeinfo == typeid(sol::lua_value*) ||
            typeinfo == typeid(const sol::lua_value*) ||
            typeinfo == typeid(sol::lua_value const*)){
        MR_ERROR("ERROR: sdmp::Value can't use sol::lua_value directly, "
                 "plese use Value(sol::table) or Value(sol::function) for sol data,"
                 "or Value::from_lua_value(),Value::create_from_lua_value() to use it's c++ compatible data");
    }
    else{
        MR_WARN("Warnning: Value hold uncompatible type:{}",typeinfo.name());
    }
    return kPorpertyNone;
}
int32_t Value::lua_to_any(const sol::lua_value* lua_value_ptr,std::any& any_value){
    int32_t ret = 0;
    const sol::lua_value& value = *lua_value_ptr;
    auto value_type = value.value().get_type();
    switch (value_type) {
    case sol::type::number:any_value = value.as<double>();break;
    case sol::type::string:any_value = value.as<std::string>();break;
    case sol::type::boolean:any_value = value.as<bool>();break;
    case sol::type::userdata:
    case sol::type::lightuserdata:any_value = value.as<void*>();break;
    case sol::type::table:{
        auto tb = value.as<sol::table>();
        if(tb.size()){
            auto type = tb[1].get_type();
            if(type == sol::type::number){
                any_value = tb.as<std::vector<double>>();
            }
            else if(type == sol::type::string){
                any_value = tb.as<std::vector<std::string>>();
            }
            else{
                any_value = tb;
            }
        }
        else
            any_value = tb;
        break;
    }
    case sol::type::function:any_value = value.as<sol::function>();break;
    default: ret = -1;break;
    }
    return ret;
}

int32_t Value::any_to_lua(const std::any& any_value,sol::state* state,sol::lua_value* lua_value_ptr){
    int32_t ret = 0;
    auto& lua_state = *state;
    auto& type_id = any_value.type();
    if(type_id == typeid(double)) *lua_value_ptr = sol::lua_value(lua_state,ANY2F64(any_value));
    else if(type_id == typeid(std::string)) *lua_value_ptr = sol::lua_value(lua_state,ANY2STR(any_value));
    else if(type_id == typeid(bool)) *lua_value_ptr = sol::lua_value(lua_state,ANY2BOOL(any_value));
    else if(type_id == typeid(std::vector<double>)) *lua_value_ptr = sol::lua_value(lua_state,ANY2F64ARR(any_value));
    else if(type_id == typeid(std::vector<std::string>)) *lua_value_ptr = sol::lua_value(lua_state,ANY2STRARR(any_value));
    else if(type_id == typeid(void*)) *lua_value_ptr = sol::lua_value(lua_state,ANY2PTR(any_value));
    else if(type_id == typeid(sol::function)) *lua_value_ptr = sol::lua_value(lua_state,ANY2LUAFUN(any_value));
    else if(type_id == typeid(sol::table)) *lua_value_ptr = sol::lua_value(lua_state,ANY2LUATABLE(any_value));
    else {*lua_value_ptr = sol::lua_value(lua_state,0);ret = -1;}
    return ret;
}

void sdp_frame_free_packet_releaser(AVFrame*,AVPacket* packet){
    if(packet){
        av_packet_free(&packet);
    }
}
void sdp_frame_unref_packet_releaser(AVFrame*,AVPacket* packet){
    if(packet){
        av_packet_unref(packet);
    }
}
void sdp_frame_free_frame_releaser(AVFrame* frame,AVPacket*){
    if(frame){
        av_frame_free(&frame);
    }
}
void sdp_frame_unref_frame_releaser(AVFrame* frame,AVPacket*){
    if(frame){
        av_frame_unref(frame);
    }
}

void sdp_frame_free_both_releaser(AVFrame*frame, AVPacket* packet){
    if(packet){
        av_packet_free(&packet);
    }
    if(frame){
        av_frame_free(&frame);
    }
}

FramePointer sdp_frame_make_color_frame(int32_t av_format, int32_t width, int32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t a){
    AVFrame *frame = av_frame_alloc();
    frame->width = width;
    frame->height = height;
    frame->format = av_format;
    av_frame_get_buffer(frame,1);

    if (av_format == AV_PIX_FMT_RGBA || av_format == AV_PIX_FMT_BGRA) {
        int32_t pixels = width*height;
        uint32_t value = 0;
        uint8_t* ptr_value = (uint8_t*)&value;
        if(av_format == AV_PIX_FMT_RGBA){
            ptr_value[0] = b;
            ptr_value[1] = g;
            ptr_value[2] = r;
            ptr_value[3] = a;
        }
        else{
            ptr_value[0] = r;
            ptr_value[1] = g;
            ptr_value[2] = b;
            ptr_value[3] = a;
        }
        uint32_t* dest_ptr = (uint32_t*)frame->data[0];
        for (int i = 0; i < pixels; i++) {
            dest_ptr[i] = value;
        }

    }
    auto sdp_frame = Frame::make_frame(frame);
    sdp_frame->releaser = sdp_frame_free_frame_releaser;
    return sdp_frame;
}

#define PIX_FMT_NV12  23
#define PIX_FMT_NV21  24
#define PIX_FMT_GRAY8 8
#define PIX_FMT_RGBA  26
#define PIX_FMT_BGRA  28
#define PIX_FMT_RGB24 2
#define PIX_FMT_BGR24 3

#define TEST_PIXEL_FORMAT 23
FramePointer make_test_frame_from_yuv420p(std::shared_ptr<sdmp::Frame> src_frame)
{
    std::shared_ptr<sdmp::Frame> sdp_frame= std::shared_ptr<sdmp::Frame>(new sdmp::Frame());
#if (TEST_PIXEL_FORMAT > 0)

        int32_t width = src_frame->frame->width;
        int32_t height = src_frame->frame->height;
        uint8_t* y_planar = src_frame->frame->data[0];
        uint8_t* u_planar = src_frame->frame->data[1];
        uint8_t* v_planar = src_frame->frame->data[2];

        sdp_frame->frame = av_frame_alloc();
        uint8_t* buffer_temp = new uint8_t[width*height*4];
        sdp_frame->frame->opaque = buffer_temp;
        sdp_frame->releaser = [](AVFrame* frame,AVPacket*){
            uint8_t* buffer = (uint8_t*)frame->opaque;
            delete [] buffer;
            av_frame_free(&frame);
        };

        //YUV420P GRAY8 ORIGN
        AVFrame& frame = *sdp_frame->frame;
        frame.width = width;
        frame.height = height;
        frame.data[0] = y_planar;
        frame.data[1] = u_planar;
        frame.data[2] = v_planar;
        frame.linesize[0] = src_frame->frame->linesize[0];
        frame.linesize[1] = src_frame->frame->linesize[1];
        frame.linesize[2] = src_frame->frame->linesize[2];
        frame.format = AV_PIX_FMT_YUV420P;
#if (TEST_PIXEL_FORMAT == PIX_FMT_GRAY8)
        frame.format = AV_PIX_FMT_GRAY8;
#endif

        //NV12/21
#if  (TEST_PIXEL_FORMAT == PIX_FMT_NV12) || (TEST_PIXEL_FORMAT == PIX_FMT_NV21)
#if  (TEST_PIXEL_FORMAT == PIX_FMT_NV12)
        libyuv::I420ToNV12(
#elif  (TEST_PIXEL_FORMAT == PIX_FMT_NV21)
        libyuv::I420ToNV21(
#endif
               y_planar,src_frame->frame->linesize[0],
               u_planar,src_frame->frame->linesize[1],
               v_planar,src_frame->frame->linesize[2],
               buffer_temp,width,
               buffer_temp+width*height,width,
               width,height);
        frame.format = TEST_PIXEL_FORMAT;
        frame.data[0] = buffer_temp;
        frame.data[1] = buffer_temp+width*height;
        frame.linesize[0] = width;
        frame.linesize[1] = width;
#endif
        //RGBA/BGRA
#if  (TEST_PIXEL_FORMAT == PIX_FMT_RGBA) || (TEST_PIXEL_FORMAT == PIX_FMT_BGRA)
#if  (TEST_PIXEL_FORMAT == PIX_FMT_RGBA)
        libyuv::I420ToABGR(
#elif  (TEST_PIXEL_FORMAT == PIX_FMT_BGRA)
        libyuv::I420ToARGB(
#endif
               y_planar,src_frame->frame->linesize[0],
               u_planar,src_frame->frame->linesize[1],
               v_planar,src_frame->frame->linesize[2],
               buffer_temp,width*4,
               width,height);
        frame.format = TEST_PIXEL_FORMAT;
        frame.data[0] = buffer_temp;
        frame.linesize[0] = width*4;
#endif
        //RGB/BGR  I420ToRAW=RGB I420ToRGB24=BGR
#if  (TEST_PIXEL_FORMAT == PIX_FMT_RGB24) || (TEST_PIXEL_FORMAT == PIX_FMT_BGR24)
#if  (TEST_PIXEL_FORMAT == PIX_FMT_RGB24)
        libyuv::I420ToRAW(
#elif  (TEST_PIXEL_FORMAT == PIX_FMT_BGR24)
        libyuv::I420ToRGB24(
#endif
               y_planar,src_frame->frame->linesize[0],
               u_planar,src_frame->frame->linesize[1],
               v_planar,src_frame->frame->linesize[2],
               buffer_temp,width*3,
               width,height);
        frame.format = TEST_PIXEL_FORMAT;
        frame.data[0] = buffer_temp;
        frame.linesize[0] = width*3;
#endif
#endif
        return sdp_frame;
}



const char* sdp_os(){
    return SDP_OS;
}
const char* sdp_arch(){
    return SDP_ARCH;
}

int32_t ValueUtils::arguments_to_lua_values(const Arguments& args,sol::state& state,std::vector<sol::lua_value>& lua_values){
    for(const auto& item : args.params()){
        sol::lua_value v =sol::nil;
        item.to_lua_value(&state,&v);
        lua_values.push_back(v);
    }
    return lua_values.size();
}
int32_t FormatUtils::get_audio_sample_format_from_bytes(uint8_t bytes)
{
    if(bytes > 8)
        return int32_t(AV_SAMPLE_FMT_NONE);
    AVSampleFormat sample_bytes_types [] = {AV_SAMPLE_FMT_NONE,
                                            AV_SAMPLE_FMT_U8,
                                            AV_SAMPLE_FMT_S16,
                                            AV_SAMPLE_FMT_NONE,
                                            AV_SAMPLE_FMT_FLT,
                                            AV_SAMPLE_FMT_NONE,
                                            AV_SAMPLE_FMT_NONE,
                                            AV_SAMPLE_FMT_NONE,
                                            AV_SAMPLE_FMT_DBL};
    return int32_t(sample_bytes_types[bytes]);
}


void FormatUtils::to_lua_table(const Format &format, sol::table &table){
    table["type"] = format.type;
    table["codec"] = format.codec;
    table["format"] = format.format;
    table["width"] = format.width;
    table["height"] = format.height;
    table["fps"] = format.fps;
    table["channels"] = format.channels;
    table["samplerate"] = format.samplerate;
    table["bitrate"] = format.bitrate;
    table["timebase_num"] = format.timebase.numerator;
    table["timebase_dem"] = format.timebase.denominator;
    table["type_name"] = (format.type < 0) ? "null" : av_get_media_type_string((AVMediaType)format.type);
    table["codec_name"] = (format.codec < 0) ? "null" : avcodec_get_name((AVCodecID)format.codec);
    if(format.format < 0)
        table["format_name"] = std::string("null");
    else if(format.type == AVMEDIA_TYPE_VIDEO){
        auto desc = av_pix_fmt_desc_get((AVPixelFormat)format.format);
        table["format_name"] = desc->name;
        table["format_bpp"] = av_get_bits_per_pixel(desc);
    }
    else if(format.type == AVMEDIA_TYPE_AUDIO){
        table["format_name"] = av_get_sample_fmt_name((AVSampleFormat)format.format);
        table["format_bpp"] = av_get_bytes_per_sample((AVSampleFormat)format.format) * 8;
    }
    else if(format.type == AVMEDIA_TYPE_SUBTITLE){
        table["format_name"] = "n/a";
        table["format_bpp"] = -1;
    }

}

string FormatUtils::printable(const Format &format)
{
    std::ostringstream stream;
    stream<<"type:"<<(const char*)((format.type>=0)?av_get_media_type_string((AVMediaType)format.type):"unknown");
    stream<<" codec:"<<avcodec_get_name((AVCodecID)format.codec);
    stream<<" bitrate:"<<setiosflags(ios::fixed)<<setprecision(2)<<format.bitrate/1000.0<<"kbps timebase:"<<format.timebase.numerator<<"/"<<format.timebase.denominator;
    if(format.type == AVMEDIA_TYPE_VIDEO){
        auto desc = av_pix_fmt_desc_get((AVPixelFormat)format.format);
        stream<<" format:"<<(desc?desc->name:"unknown");
        stream<<" width:"<<format.width<<" height:"<<format.height<<" fps:"<<format.fps;
    }
    else if(format.type == AVMEDIA_TYPE_AUDIO){
        auto name = av_get_sample_fmt_name((AVSampleFormat)format.format);
        stream<<" format:"<<(name?name:"unknown");
        stream<<" channels:"<<format.channels<<" samplerate:"<<format.samplerate;
    }

    return stream.str();
}

int32_t FormatUtils::sample_format_to_codec_id(int sample_format)
{
    AVSampleFormat packeted_format = av_get_packed_sample_fmt((AVSampleFormat)sample_format);
    AVCodecID pcm_codec = AV_CODEC_ID_NONE;
    if(packeted_format == AV_SAMPLE_FMT_DBL)
        pcm_codec = AV_CODEC_ID_PCM_F64LE;
    if(packeted_format == AV_SAMPLE_FMT_FLT)
        pcm_codec = AV_CODEC_ID_PCM_F32LE;
    if(packeted_format == AV_SAMPLE_FMT_S32)
        pcm_codec = AV_CODEC_ID_PCM_S32LE;
    if(packeted_format == AV_SAMPLE_FMT_S16)
        pcm_codec = AV_CODEC_ID_PCM_S16LE;
    if(packeted_format == AV_SAMPLE_FMT_U8)
        pcm_codec = AV_CODEC_ID_PCM_U8;

    return pcm_codec;
}


std::vector<uint8_t> StringUtils::string_to_vector(const std::string& str){
    return std::vector<uint8_t>(str.begin(),str.end());
}

std::string StringUtils::vector_to_string(const std::vector<uint8_t> buf){
    return  std::string(buf.begin(),buf.end());
}

std::vector<std::string> StringUtils::splite_string(const std::string& str,char spliter){
    vector<string> strings;
    istringstream stream(str);
    string splited;
    while (std::getline(stream, splited, spliter)) {
        strings.push_back(splited);
    }
    return strings;
}

inline std::vector<uint8_t> StringUtils::hex_to_bytes(const std::string& hex) {
  std::vector<uint8_t> bytes;
  size_t length = hex.size();
  if(length != 0 && length % 2 == 0){
      for (size_t index = 0; index < hex.length(); index += 2) {
        std::string byte_string = hex.substr(index, 2);
        uint8_t byte = (uint8_t) strtol(byte_string.c_str(), NULL, 16);
        bytes.push_back(byte);
      }
  }
  return bytes;
}


inline std::string StringUtils::bytes_to_hex(const std::vector<uint8_t>& bytes) {
  return fmt::format("{}",spdlog::to_hex(bytes));
}

std::string StringUtils::fourcc(uint32_t fourcc){
    char* fourcc_bytes = (char*)&fourcc;
    return fmt::format("{}{}{}{}",fourcc_bytes[0], fourcc_bytes[1], fourcc_bytes[2],fourcc_bytes[3]);
}

std::string StringUtils::printable(const Value& value) {
    const char* type_names[]{
        "None",
        "Number",
        "String",
        "Bool",
        "NumberArray",
        "StringArray",
        "Pointer",
        "LuaFunction",
        "LuaTable"
    };
    if(!value.valid_sdmp_value())
        return fmt::format("unknown value type:{}",value.value_.type().name());

    auto type_ = value.type_;
    const char* type_name = type_names[type_];

    if(type_ == ValueType::kPorpertyNumber)
        return fmt::format("{}:{}",type_name,ANY2F64(value.value_));
    else if(type_ == ValueType::kPorpertyBool)
        return fmt::format("{}:{}",type_name,ANY2BOOL(value.value_));
    else if(type_ == ValueType::kPorpertyString)
        return fmt::format("{}:{}",type_name,ANY2STR(value.value_));
    else if(type_ == ValueType::kPorpertyNumberArray){
        return fmt::format("{}:{}",type_name,fmt::join(ANY2F64ARR(value.value_), ","));
    }
    else if(type_ == ValueType::kPorpertyStringArray){
        return fmt::format("{}:{}",type_name,fmt::join(ANY2STRARR(value.value_), ","));
    }
    else if(type_ == ValueType::kPorpertyPointer)
        return fmt::format("{}:{}",type_name,ANY2PTR(value.value_));
    else if(type_ == ValueType::kPorpertyLuaFunction)
        return fmt::format("{}:{}",type_name,ANY2LUAFUN(value.value_).valid());
        //ss<<"Function:"<<(ANY2LUAFUN(value_).valid()?"valid":"nil");
    else if(type_ == ValueType::kPorpertyLuaTable)
        return fmt::format("{}:{}",type_name,ANY2LUATABLE(value.value_).valid());
        //ss<<"Table:"<<(ANY2LUATABLE(value_).valid()?"valid":"nil");
    return "unknown";
}

BitStreamConvert::BitStreamConvert(const std::string &type)
    :type_(type)
{

}

BitStreamConvert::~BitStreamConvert()
{
    close();
}

bool BitStreamConvert::create(AVCodecParameters *param)
{
    close();
    int ret = 0;
    AVBSFContext* context = nullptr;
    do{
        const AVBitStreamFilter * filter = av_bsf_get_by_name(type_.c_str());
        ret = av_bsf_alloc(filter,&context);
        if(ret < 0)
            break;
        ret = avcodec_parameters_copy(context->par_in,param);
        if(ret < 0)
            break;
        ret = av_bsf_init(context);
        if(ret < 0){
            break;
        }
        std::vector<uint8_t> buf(context->par_out->extradata,context->par_out->extradata+context->par_out->extradata_size);
        auto s = StringUtils::bytes_to_hex(buf);
        MR_LOG_DEAULT("context->par_out->extradata: {}",s.c_str());
        context_ = context;
    }while(false);

    if(context_ == nullptr){
        if(context)
            av_bsf_free(&context);
        return false;
    }

    return true;
}

AVCodecParameters *BitStreamConvert::params_out()
{
    if(context_ == nullptr)
        return nullptr;
    return ((AVBSFContext*)context_)->par_out;
}

bool BitStreamConvert::convert(AVPacket* packet)
{
    if(context_ == nullptr)
        return false;

    int ret = av_bsf_send_packet((AVBSFContext*)context_,packet);
    if(ret < 0)
        return false;

    do{
        AVPacket* converted_pack = av_packet_alloc();
        ret = av_bsf_receive_packet((AVBSFContext*)context_,converted_pack);
        if(ret < 0){
            av_packet_free(&converted_pack);
            if(ret == AVERROR(EAGAIN)){
                return true;
            }
            return false;
        }
        FramePointer new_frame = FramePointer(new Frame());
        new_frame->releaser = sdp_frame_free_packet_releaser;
        new_frame->packet = converted_pack;
        converted_packets_.push(new_frame);
    }while(true);
    return true;
}

int32_t BitStreamConvert::converted_count()
{
    return converted_packets_.size();
}

FramePointer BitStreamConvert::pop()
{
    FramePointer frame;
    if(converted_packets_.size()){
        frame = converted_packets_.front();
        converted_packets_.pop();
    }
    return frame;
}

void BitStreamConvert::close()
{
    if(context_){
        AVBSFContext* context = (AVBSFContext*)context_;
        av_bsf_free(&context);
        context_ = nullptr;
    }
}


int32_t MediaInfoProbe::get_info(const std::string &media, Info &info)
{
    info.duration = 0;
    info.video_format = Format();
    info.audio_formats.clear();

    auto media_file_ = avformat_alloc_context();
    int ret = avformat_open_input(&media_file_,media.c_str(),nullptr,nullptr);
    if(ret < 0){
        return -1;
    }

    ret = avformat_find_stream_info(media_file_,nullptr);
    if(ret < 0) {
        return -2;
    }

    size_t streams = media_file_->nb_streams;
    for(size_t index = 0; index < streams; index++){
        auto stream = media_file_->streams[index];
        sdmp::Format format;
        format.codec = stream->codecpar->codec_id;
        format.format = stream->codecpar->format;
        format.type = stream->codecpar->codec_type;
        if(format.type == AVMEDIA_TYPE_AUDIO){
            format.samplerate = stream->codecpar->sample_rate;
            format.channels   = stream->codecpar->ch_layout.nb_channels;
            info.audio_formats.push_back(format);
        }
        else if(format.type == AVMEDIA_TYPE_VIDEO){
            format.width = stream->codecpar->width;
            format.height= stream->codecpar->height;
            format.fps = FFRational(stream->avg_frame_rate).as_double();
            info.video_format = format;
        }
        else if(format.type == AVMEDIA_TYPE_SUBTITLE) {

        }
        else{
            continue;
        }
    }

    info.bitrate = media_file_->bit_rate;
    FFTimestamp duration(media_file_->duration);
    info.duration = duration.seconds() * 1000;

    FFTimestamp timestamp = FFTimestamp(std::chrono::milliseconds(info.duration-1000));
    int64_t pos = timestamp.rescale(FFTimeBaseQ);

    ret = av_seek_frame(media_file_,-1,pos,AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
    if( ret >= 0){
        AVPacket packet = {0};
        ret = av_read_frame(media_file_,&packet);
        if(ret >= 0){
            AVStream* stream = media_file_->streams[packet.stream_index];
            int32_t pts = 1000.0 * packet.pts * av_q2d(stream->time_base);
            info.duration = std::max(info.duration,pts);
        }
    }

    avformat_close_input(&media_file_);
    return 0;
}

}
