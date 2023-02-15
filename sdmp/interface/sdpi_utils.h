#ifndef SDP_UTILS_H
#define SDP_UTILS_H
#include <inttypes.h>
#include <string.h>
#include <vector>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include "sdpi_basic_types.h"

extern "C"{
    struct AVRational;
}

typedef struct AVCodecParameters AVCodecParameters;

#define CHECK_ANNEXB_START_CODE(data) (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3]==1)

namespace mr::sdmp {

void sdp_frame_free_packet_releaser(AVFrame*, AVPacket* packet);
void sdp_frame_unref_packet_releaser(AVFrame*,AVPacket* packet);
void sdp_frame_free_frame_releaser(AVFrame*frame, AVPacket*);
void sdp_frame_unref_frame_releaser(AVFrame* frame,AVPacket*);
void sdp_frame_free_both_releaser(AVFrame*frame, AVPacket* packet);
FramePointer sdp_frame_make_color_frame(int32_t av_format,int32_t width,int32_t height,uint8_t r,uint8_t g,uint8_t b,uint8_t a);
FramePointer make_test_frame_from_yuv420p(std::shared_ptr<sdmp::Frame> src_frame);

const char* sdp_os();
const char* sdp_arch();

namespace FormatUtils {
    //AVSampleFormat to AVCodecID
    int32_t sample_format_to_codec_id(int sample_format);
    int32_t get_audio_sample_format_from_bytes(uint8_t bytes);

    void to_lua_table(const sdmp::Format& format, sol::table& table_ptr);
    std::string printable(const sdmp::Format& format);
};
//helper function to convert vector/string
namespace StringUtils{
    std::vector<uint8_t> string_to_vector(const std::string& str);
    std::string vector_to_string(const std::vector<uint8_t> buf);
    std::vector<std::string> splite_string(const std::string &str, char spliter);
    std::vector<uint8_t> hex_to_bytes(const std::string& hex);
    std::string bytes_to_hex(const std::vector<uint8_t>& bytes);
    std::string fourcc(uint32_t fourcc);

    std::string printable(const Value& value);
};

namespace BufferUtils{
    template<typename T>
    std::shared_ptr<T> create_shared_object_buffer(size_t count){
        std::shared_ptr<T> object_buffer = std::shared_ptr<T>(new T[count],std::default_delete<T[]>());
        return object_buffer;
    }

    inline std::shared_ptr<uint8_t> create_shared_buffer(size_t size){
        std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>(new uint8_t[size],std::default_delete<uint8_t[]>());
        return buffer;
    }
};

//a helper class to get objects from buffer
class BufferObject{
public:
    BufferObject(int32_t size){
        if(size)
            buffer_ = new uint8_t[size];
    }
    ~BufferObject(){
        if(buffer_)
            delete [] buffer_;
    }
    template<typename T> T* as(){
        return (T*)buffer_;
    }
private:
    uint8_t* buffer_ = nullptr;
};

//a helper class to do PCM layout-convert/scale
namespace PcmConvertor
{
    template <typename SampleType>
    static int32_t planar_to_packed(uint8_t** planar_source , uint8_t* packed_dest,uint8_t channels,uint32_t samples)
    {
        SampleType* dest_sample = (SampleType*)packed_dest;
        SampleType** source_sample = (SampleType**)planar_source;
        for(int sample_index = 0; sample_index < samples;sample_index++)
        {
            for (int channel_index=0; channel_index < channels; channel_index++)
            {
                *dest_sample = *(source_sample[channel_index] + sample_index);
                dest_sample ++;
            }
        }
        return samples;
    }

    template <typename SampleType>
    static int32_t packet_to_planar(uint8_t* packed_source,uint8_t** planar_dest ,uint8_t channels,uint32_t samples)
    {
        SampleType* source_sample = (SampleType*)packed_source;
        SampleType** dest_sample = (SampleType**)planar_dest;
        for(uint32_t sample_index = 0; sample_index < samples;sample_index++)
        {
            for (uint32_t channel_index=0; channel_index < channels; channel_index++)
            {
                *(dest_sample[channel_index] + sample_index) = *source_sample;
                source_sample ++;
            }
        }
        return samples;
    }

    template <typename SampleType>
    static int32_t scale(float scale,uint8_t* packed_source,int32_t samples){
        SampleType* source_sample = (SampleType*)packed_source;
        for(uint32_t sample_index = 0; sample_index < samples;sample_index++)
        {
            *source_sample = *source_sample * scale;
            source_sample++;
        }
        return 0;
    }

    template<typename InType,typename OutType>
    static int32_t valid_channel_mapping(const std::vector<InType>& input_mapping,std::vector<OutType>& valid_mapping,int32_t channels){
        valid_mapping.clear();
        for(int32_t index = 0; index < channels ; index++){
            int mapping_index = ((index >= input_mapping.size()) ? index : input_mapping[index]);
            int mapping_valid_index = (mapping_index >= channels) ? index : mapping_index;
            valid_mapping.push_back(mapping_valid_index);
        }
        return 0;
    }

    template <typename SampleType>
    static int32_t remapping_packeted(const uint8_t* pcm_source,uint8_t* pcm_dest,int32_t samples,int32_t channels,const std::vector<int32_t>& mapping){
        SampleType* source_sample = (SampleType*)pcm_source;
        SampleType* dest_sample = (SampleType*)pcm_dest;
        for(uint32_t sample_index = 0; sample_index < samples;sample_index++)
        {
            for (uint32_t channel_index=0; channel_index < channels; channel_index++)
            {
                int mapping_ch = mapping[channel_index];
                if(mapping_ch >= 0 && mapping_ch < channels)
                    dest_sample[channel_index] = source_sample[mapping_ch];
                else
                    dest_sample[channel_index] = 0;
            }
            source_sample += channels;
            dest_sample += channels;
        }
        return 0;
    }

    template <typename SampleType>
    static int32_t remapping_planner(const uint8_t** pcm_source,uint8_t** pcm_dest,int32_t samples,int32_t channels,int mapping[]){

        int channel_bytes_all = sizeof (SampleType) * samples;
        for (uint32_t channel_index=0; channel_index < channels; channel_index++)
        {
            int mapping_ch = mapping[channel_index];
            if(mapping_ch >= 0 && mapping_ch < channels){
                memcpy(pcm_dest[channel_index],pcm_source[mapping_ch],channel_bytes_all);
            }
            else{
                memset(pcm_dest[channel_index],0,channel_bytes_all);
            }
        }
        return 0;
    }
};

class BitStreamConvert{
public:
    BitStreamConvert(const std::string& type);
    ~BitStreamConvert();

    bool create(AVCodecParameters* param);
    AVCodecParameters* params_out();
    bool convert(AVPacket *packet);
    int32_t converted_count();
    FramePointer pop();
private:
    void close();
private:
    std::string type_;
    void* context_ = nullptr;
    std::queue<FramePointer> converted_packets_;
};

class MediaInfoProbe{
public:
    struct Info{
        int32_t duration = 0;
        int32_t bitrate  = 0;
        Format video_format;
        std::vector<Format> audio_formats;
    };
public:
    int32_t get_info(const std::string& media,Info& info);
};

class StatusWait{
public:
    void request_and_wait(std::condition_variable* condition,int32_t wait_milliseconds = 1){
        request_ = true;
        while (!statused_) {
            if(condition)
                condition->notify_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_milliseconds));
        }
        request_ = false;
    }

    bool check(){
        if(request_){
             request_ = false;
             statused_ = true;
        }
        return statused_;
    }
    bool statused(){
        return statused_;
    }
    void reset(){
        request_ = false;
        statused_ = false;
    }
private:
    bool request_ = false;
    bool statused_ = false;
};

template<typename T>
class Queue{
public:

    int32_t push(T frame)
    {
        std::lock_guard<std::mutex> lock(queue_lock_);
        frames_.push(frame);
        return 0;
    }
    T pop()
    {
        T frame;
        std::lock_guard<std::mutex> lock(queue_lock_);
        if(frames_.size()){
            frame = frames_.front();
            frames_.pop();
        }
        return frame;
    }
    int32_t pop_each(std::function<void(T&)> method)
    {
        //method may be run a lot of time, so use temp to unlock queue
        std::queue<T> frames;
        {
            std::lock_guard<std::mutex> lock(queue_lock_);
            if(frames_.empty())
                return 0;
            frames = frames_;
            frames_ = std::queue<T>();
        }

        while (frames.size()) {
            auto& frame = frames.front();
            method(frame);
            frames.pop();
        }
        return 0;
    }
    int32_t move(std::queue<T>& other)
    {
        while (frames_.size()) {
            auto frame = frames_.front();
            other.push(frame);
            frames_.pop();
        }
        return 0;
    }
    int32_t size()
    {
        std::lock_guard<std::mutex> lock(queue_lock_);
        return frames_.size();
    }
    int32_t clear()
    {
        std::lock_guard<std::mutex> lock(queue_lock_);
        frames_ = std::queue<T>();
        return 0;
    }
private:
    std::queue<T> frames_;
    std::mutex queue_lock_;
};

}


#endif // SDP_UTILS_H
