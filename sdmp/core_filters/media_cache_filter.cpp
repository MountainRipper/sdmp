#include <fstream>
#include <iomanip>
#include <list>
#include <digest/digestpp.hpp>
#include "media_cache_filter.h"
#include "nlohmann/json.hpp"
using namespace nlohmann;

namespace mr::sdmp {

COM_REGISTER_OBJECT(MediaCacheSaverFilter)


class MediaCacheManager{   
public:
    struct Segment{
        int32_t     start_ms_       = 0;
        int32_t     end_ms_         = 0;
        int32_t     start_pts_      = 0;
        int32_t     end_pts_        = 0;
        bool        is_continued_   = 0;
        std::string cache_file_;
    };
    int32_t init(const std::string &cache_dir, std::string uri_key)
    {
        cache_dir_ = cache_dir;
        uri_original_ = uri_key;
        uri_md5_ = digestpp::md5().absorb(uri_original_).hexdigest();

        open_mainfest();
        return 0;
    }

    int32_t create_fragment(IFilterExtentionMediaCacheSource::Info info,int32_t pts_from){
        finish_fragment();

        // segment is mp4 file?
        std::string cache_file = cache_dir_ + uri_md5_ + ".mp4";
        MR_LOG_DEAULT("cached file name ======>>>>>>{}", cache_file);
        const char* out_filename = cache_file.c_str();
        int ret = 0;
        avformat_alloc_output_context2(&muxer_, NULL, NULL, out_filename);
        if (!muxer_) {
            fprintf(stderr, "Could not create output context\n");
            ret = AVERROR_UNKNOWN;
            //goto end;
            return -1;
        }

        stream_mapping_.clear();

        auto ofmt = muxer_->oformat;

        int32_t stream_index = 0;
        for (size_t input_index = 0; input_index < info.output_pins.size(); input_index++) {
            auto pin = info.output_pins[input_index];
            auto& formats = pin->formats();
            if(formats.empty())
                return -1;
            AVCodecParameters *in_codecpar = (AVCodecParameters*)formats[0].codec_parameters;

            if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
                in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
                in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
                stream_mapping_[pin->index()] = -1;
                continue;
            }

            stream_mapping_[pin->index()] = stream_index++;

            auto out_stream = avformat_new_stream(muxer_, NULL);
            if (!out_stream) {
                fprintf(stderr, "Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                return -2;
            }

            ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
            if (ret < 0) {
                fprintf(stderr, "Failed to copy codec parameters\n");
                return -3;
            }
            out_stream->codecpar->codec_tag = 0;
        }

        if (!(muxer_->flags & AVFMT_NOFILE)) {
            ret = avio_open(&muxer_->pb, out_filename, AVIO_FLAG_WRITE);
            if (ret < 0) {
                return ret;
            }
        }

        av_dump_format(muxer_, 0, out_filename, 1);

        ret = avformat_write_header(muxer_, NULL);
        if (ret < 0) {
            return ret;
        }

//        muxer_->streams[0]->time_base;

        Segment segment;
        segment.start_ms_       = pts_from;
        segment.end_ms_         = 212;
        segment.start_pts_      = pts_from;
        segment.end_pts_        = 67890;
        segment.is_continued_   = false;
        segment.cache_file_     = cache_file;
        segments_.push_back(segment);

        write_manifest();
        has_segment(1);
        return 0;
    }
    int32_t finish_fragment(){
        if(muxer_){
            av_write_trailer(muxer_);
            if (!(muxer_->oformat->flags & AVFMT_NOFILE))
                avio_closep(&muxer_->pb);
            avformat_free_context(muxer_);
            muxer_ = nullptr;
        }
        return 0;
    }

    int32_t write_frame(AVPacket* packet,int32_t pin_index){
        if(muxer_){
            auto it = stream_mapping_.find(pin_index);
            if(it == stream_mapping_.end())
                return -1;

            AVPacket* packet_clone = av_packet_clone(packet);
            packet_clone->stream_index = it->second;
            auto tb = muxer_->streams[packet_clone->stream_index]->time_base;
            packet_clone->pts = av_rescale_q(packet->pts,{1,1000},muxer_->streams[packet_clone->stream_index]->time_base);
            packet_clone->dts = av_rescale_q(packet->dts,{1,1000},muxer_->streams[packet_clone->stream_index]->time_base);

            MR_LOG_DEAULT("stream:{} pts:{} dts:{} timebase:{}/{}",packet_clone->stream_index,packet_clone->pts,packet_clone->dts,tb.num,tb.den );
            av_interleaved_write_frame(muxer_,packet_clone);
            av_packet_free(&packet_clone);
        }
        return 0;
    }


    std::string has_cached_file(){
        std::ifstream fstream;
        fstream.open(cache_dir_ + uri_md5_ + ".mp4");
        if(!fstream.is_open()) {
            return "not exist such mp4 file";
        }
        return cache_dir_ + uri_md5_ + ".mp4";
    }

    std::string has_cache(int32_t ms){
        std::ifstream fstream;
        fstream.open(cache_dir_ + uri_md5_ + ".json");
        if(!fstream.is_open()) {
            return "not exist such json file";
        }
        return cache_dir_ + uri_md5_ + ".json";
    }

    std::string has_segment(int32_t ms){
        for(const auto& segment : segments_) {
            if(segment.start_ms_ < ms && segment.end_ms_ < ms) {
                return segment.cache_file_;
            }
        }
        return "";
    }
private:
    int32_t open_mainfest() {
        segments_.clear();
        std::ifstream fstream;
        fstream.open(cache_dir_ + uri_md5_ + ".json");
        if(!fstream.is_open()) {
            return -1;
        }
        try {
            manifest_ = nlohmann::json::parse(fstream,nullptr,true,true);
        }  catch (nlohmann::json::parse_error& e) {
            MR_ERROR("open_mainfest() Error: {} exception id:{} byte position of error:{}",e.what(),e.id, e.byte);
            return -1;
        }
        auto & segments = manifest_["segments"];
        for(const auto& segment_json : segments){
            Segment segment;
            segment.start_ms_     = segment_json["start_ms"]     ;
            segment.end_ms_       = segment_json["end_ms"]       ;
            segment.start_pts_    = segment_json["start_pts"]    ;
            segment.end_pts_      = segment_json["end_pts"]      ;
            segment.is_continued_ = segment_json["is_continured"];
            segment.cache_file_   = segment_json["cache_file"]   ;
            segments_.push_back(segment);
        }
        return 0;
    }

    int32_t write_manifest(){
        json menifest_json;
        json segments_json;
        for(const auto& segment : segments_){
            json segment_json;
            segment_json["start_ms"]     =segment.start_ms_    ;
            segment_json["end_ms"]       =segment.end_ms_      ;
            segment_json["start_pts"]    =segment.start_pts_   ;
            segment_json["end_pts"]      =segment.end_pts_     ;
            segment_json["is_continured"]=segment.is_continued_;
            segment_json["cache_file"]   =segment.cache_file_  ;
            segments_json.push_back(segment_json);
        }

        menifest_json["segments"] = segments_json;
        menifest_json["uri"] = uri_original_;

        std::ofstream fstream;
        fstream.open(cache_dir_ + uri_md5_ + ".json");
        if(!fstream.is_open()){
            return -1;
        }
        fstream<<std::setw(4)<<menifest_json;
        fstream.close();
        return 0;
    }
private:
    std::string cache_dir_;
    std::string uri_original_;
    std::string uri_md5_;
    std::map<int32_t,int32_t> stream_mapping_;
    AVFormatContext* muxer_ = nullptr;
    std::list<Segment> segments_;
    json  manifest_;
};


MediaCacheSaverFilter::MediaCacheSaverFilter()
{
    manager_ = std::shared_ptr<MediaCacheManager>(new MediaCacheManager());
}

int32_t MediaCacheSaverFilter::initialize(IGraph *graph, const Value &filter_values)
{
    GeneralFilter::initialize(graph,filter_values);
    return 0;
}

int32_t MediaCacheSaverFilter::connect_before_match(IFilter *sender_filter)
{
    if(sender_filter_ == nullptr ){
        ComPointer<IFilter> filter(sender_filter);
        cache_source_ = filter.As<IFilterExtentionMediaCacheSource>();
        if(!cache_source_){
            MR_ERROR("MediaCacheSaveFilter::connect_before_match sender not support IFilterExtentionMediaCacheSource");
            return kErrorFilterNoInterfce;
        }
        sender_filter_ = sender_filter;

        auto ret = cache_source_->get_source_media_info(source_info_);
        if(ret < 0){
            MR_ERROR("MediaCacheSaveFilter::connect_before_match get source media info failed");
            return kErrorFilterInvalid;
        }

        //init the cache manager with uri key
        manager_->init(properties_["cacheDir"],source_info_.uri_key);

        if(source_info_.output_pins.empty()){
            MR_ERROR("MediaCacheSaveFilter::connect_before_match get empty output pins");
            return kErrorConnectFailedNoOutputPin;
        }
        if(source_info_.uri_key.empty() || source_info_.protocol.empty() || source_info_.format_name.empty()){
            MR_ERROR("MediaCacheSaveFilter::check_input_format get source media info not complate: uri_key:{} protoco;:{} format_name:{}",
                      source_info_.uri_key.c_str() , source_info_.protocol.c_str() , source_info_.format_name.c_str());
            return kErrorInvalidParameter;
        }

        pass_though_mode_ = (source_info_.protocol == "file");
        if(pass_though_mode_){
            MR_INFO("MediaCacheSaveFilter::connect_before_match source is local file use pass though mode");
        }
        //cache_source_->active_cache_mode(!pass_though_mode_);

        for(auto pin : source_info_.output_pins){
            auto input_pin  = create_general_pin(kInputPin,pin->formats());
            auto output_pin = create_general_pin(kOutputPin,pin->formats());
            source_pin_map_[pin] = std::pair<IPin*,IPin*>(input_pin.Get(),output_pin.Get());
        }
        sync_pins_to_script(kInputPin);
        sync_pins_to_script(kOutputPin);

        manager_->create_fragment(source_info_, 0);
    }
    if(sender_filter_ != nullptr && sender_filter_ != sender_filter){
        MR_ERROR("MediaCacheSaveFilter::check_input_format not from same sender");
        return -1;
    }
    return 0;
}

int32_t MediaCacheSaverFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    auto it = source_pin_map_.find(sender_pin);
    if(it == source_pin_map_.end())
        return kErrorConnectFailedNotMatchInOut;

    return 0;
}

int32_t MediaCacheSaverFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t MediaCacheSaverFilter::receive(IPin *input_pin, FramePointer frame)
{
    auto it = source_pin_map_.find(input_pin->sender());
    if(it == source_pin_map_.end())
        return kErrorConnectFailedNotMatchInOut;
    //TODO write to cache
    if(frame->flag & kFrameFlagEos) {
        manager_->finish_fragment();
    }else {
        auto sender_pin = input_pin->sender();
        manager_->write_frame(frame->packet, sender_pin->index());
    }

    it->second.second->deliver(frame);
    return 0;
}

int32_t MediaCacheSaverFilter::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    return duration;
}


int32_t MediaCacheSaverFilter::process_command(const std::string &command, const Value &param)
{
    GeneralFilter::process_command(command,param);
    if(command == kGraphCommandStop){
        if(sender_filter_){

        }
        sender_filter_ = nullptr;
        remove_pin(kInputPin,kPinIndexAll);
        remove_pin(kOutputPin,kPinIndexAll);
    }
    if(command == kGraphCommandPause){

    }
    if(command == kGraphCommandPlay){

    }if(command == kGraphCommandSeek){
        //TODO: check if has cache file, is found open and read
        //if not cached, let source filter seek and read
        //also, create a cache file to save content

    }
    return 0;
}

std::string MediaCacheSaverFilter::get_cache_file_of_uri(std::string uri)
{
    return "";
}

int32_t sdmp::MediaCacheSaverFilter::property_changed(const std::string &property, const Value &value)
{
    return 0;
}

Value sdmp::MediaCacheSaverFilter::call_method(const std::string &method, const Value &param)
{
    if(method == "getCachedFileOfUri")
    {
        return get_cache_file_of_uri(param.as_string());
    }
    return 0;
}

}



