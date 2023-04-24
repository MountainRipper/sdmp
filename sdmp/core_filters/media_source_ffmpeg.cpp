#include <chrono>
#include <sdmpi_factory.h>
#include "media_source_ffmpeg.h"


namespace mr::sdmp {

COM_REGISTER_OBJECT(MediaSourceFFmpegFilter)


MediaSourceFFmpegFilter::MediaSourceFFmpegFilter()
{    
    avformat_network_init();
    timeout_timer_ = MR_TIMER_NOW;
    timeout_check_ms_ = double(properties_["timeout"]);
}

MediaSourceFFmpegFilter::~MediaSourceFFmpegFilter()
{
    destructor_ = true;
    close();
}

int32_t MediaSourceFFmpegFilter::open_media(const std::string &uri, bool reconnect_in_proc)
{
    if(reconnect_in_proc)
        close_context();
    else
        close();
    network_timeout_ = false;
    quit_reading_flag_ = false;

    MR_LOG_DEAULT("MediaSourceFFmpegFilter::open_media : {} ", uri.data());
    uri_ = uri;
    if(!uri_.size()) {
        MR_LOG_DEAULT("MediaSourceFFmpegFilter::open_media media source uri is empty skipping....");
        return 0;
    }
    put_property_to_script("uri", uri_);

    std::string protocol;
    {
        //get protocol
        char proto[16];
        av_url_split(proto,16,0,0,0,0,0,0,0,uri.c_str());
        if(proto[0] == '\0')
            strcpy(proto,"file");
        protocol = proto;
        set_property("protocol",protocol);
    }
    media_file_ = avformat_alloc_context();
    media_file_->interrupt_callback.opaque = this;
    media_file_->interrupt_callback.callback = [](void* opaque){
        MediaSourceFFmpegFilter* owner = (MediaSourceFFmpegFilter*)opaque;
        return owner->check_timeout();
    };

    std::string final_uri = uri;
    auto custom_io_clsid = properties_["customIO"].as_string();
    if(!custom_io_clsid.empty()){
        auto custom_io_check_function = properties_["customIOCheckHandler"].as<sol::function>();
        if(custom_io_check_function.valid()){
            std::string io_uri = custom_io_check_function(id_,final_uri);
            if(!io_uri.empty()){
                //get custom io uri ,active it
                final_uri = io_uri;
                TGUID guid(custom_io_clsid.c_str());
                auto obj = Factory::create_object(guid);
                if(obj.QueryInterface(&custom_io_) == S_OK){
                    custom_io_->use(media_file_,final_uri);
                }
            }
        }
    }

    AVDictionary *options = nullptr;
    if(protocol.find("rtmp") != std::string::npos || protocol.find("rtsp") != std::string::npos)
        av_dict_set_int(&options,"timeout",properties_["timeout"].as_double() / 1000/1000,0);
    else
        av_dict_set_int(&options,"timeout",properties_["timeout"].as_double() * 1000,0);
    reset_timeout();
    int ret = avformat_open_input(&media_file_,final_uri.c_str(),nullptr,&options);
    av_dict_free(&options);
    if(ret < 0){
        char msg[128];
        av_make_error_string(msg,128,ret);
        //avformat_open_input:Note that a user-supplied AVFormatContext will be freed on failure.
        media_file_ = nullptr;
        graph_->emit_error(id_, kErrorResourceOpen);
        return -1;
    }    
    if(quit_reading_flag_){
        MR_WARN("MediaSourceFFmpegFilter::open_media check closed reading!");
        return -1;
    }

    reset_timeout();
    ret = avformat_find_stream_info(media_file_,nullptr);
    if(ret < 0) {
        graph_->emit_error(id_, kErrorResourceRead);
        return -2;
    }

    size_t streams = media_file_->nb_streams;
    for(size_t index = 0; index < streams; index++){
        auto stream = media_file_->streams[index];
        sdmp::Format format;
        format.codec = stream->codecpar->codec_id;
        format.format = stream->codecpar->format;
        format.type = stream->codecpar->codec_type;
        format.codec_parameters = stream->codecpar;
        format.bitrate = stream->codecpar->bit_rate;
        //all output has scale to 1/1000 in millisecond unit
        format.timebase = {1,1000};
        if(format.type == AVMEDIA_TYPE_AUDIO){
            format.samplerate = stream->codecpar->sample_rate;
            format.channels   = stream->codecpar->ch_layout.nb_channels;
        }
        else if(format.type == AVMEDIA_TYPE_VIDEO){
            format.width = stream->codecpar->width;
            format.height= stream->codecpar->height;
            if(stream->avg_frame_rate.num+stream->avg_frame_rate.den==0)
                format.fps = 30;
            else
                format.fps = FFRational(stream->avg_frame_rate).as_double();
            set_property("fps",format.fps);
        }
        else if(format.type == AVMEDIA_TYPE_SUBTITLE) {

        }
        else{
            continue;
        }
        if(!reconnect_in_proc){
            auto new_pin = create_general_pin(kOutputPin, format);
            stream_pins_map_[stream->index] = new_pin;
        }
    }

    FFTimestamp duration(media_file_->duration);
    set_property("duration",duration.seconds()*1000,false);

    if(!reconnect_in_proc){
        bind_pins_to_script(kOutputPin);
        reading_thread_ = std::thread(&MediaSourceFFmpegFilter::reading_proc,this);
    }
    reconnect_count_ = 10;
    media_loaded_ = true;
    return 0;
}

int32_t MediaSourceFFmpegFilter::initialize(IGraph *graph, const Value &filter_values)
{
    GeneralFilter::initialize(graph,filter_values);
    std::string uri = filter_state_.get_or("uri",std::string());
    MR_INFO("MediaSourceFFmpegFilter::init with uri : {} ", uri.c_str());
    if(uri.size())
        open_media(uri);

    return 0;
}

int32_t MediaSourceFFmpegFilter::process_command(const std::string &command, const Value& param)
{
    std::string real_command = command;
    if(caching_mode_){
        if(command == kGraphCommandSeek ||command == kGraphCommandPlay)
            return 0;
        if(command == kMediaCacheCommandPlay){
            real_command = command;
            //TODO: read
        }
    }
    GeneralFilter::process_command(command,param);
    int32_t ret = 0;
    do {
        if(real_command == kGraphCommandPause){
            pause_wait_.request_and_wait(&read_condition_);
            if (media_file_)
                av_read_pause(media_file_);
        }
        else if(real_command == kGraphCommandStop){
            close();
        }
        else if(real_command == kGraphCommandSeek){
            reset_position();
            current_read_pts_ = param.as_double();
            ret = do_seek(current_read_pts_);
        }
        else if(real_command == kGraphCommandPlay){
            pause_wait_.reset();
            if (media_file_)
                av_read_play(media_file_);
        }
    } while(false);

    return ret;
}

int32_t MediaSourceFFmpegFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    return 0;
}

int32_t MediaSourceFFmpegFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    return 0;
}

int32_t MediaSourceFFmpegFilter::receive(IPin* input_pin,FramePointer frame)
{
    return 0;
}

int32_t MediaSourceFFmpegFilter::requare(int32_t duration,const std::vector<PinIndex>& output_pins)
{
    if(media_file_ == nullptr)
        return kErrorFilterInvalid;
    if(status_ == kStatusEos)
        return kErrorFilterEos;

    int32_t pre_buffer = properties_["preBufferMs"].as_int32();
    int32_t want_read_to = current_read_pts_ + duration + pre_buffer;

    if(want_read_to > request_read_to_)
        request_read_to_ = want_read_to;
    read_condition_.notify_one();
    //MR_LOG_DEAULT("MediaSourceFFmpegFilter::requare: cur:{} need {} pre-buffer:{} read to:{}", current_read_pts_,duration,pre_buffer,request_read_to_);
    return 0;
}

int32_t MediaSourceFFmpegFilter::property_changed(const std::string& name,Value& symbol)
{
    int ret = 0;
    if(name == "uri"){
        MR_LOG_DEAULT("MediaSourceFFmpegFilter uri changed: {} ", StringUtils::printable(symbol));
        const auto& new_uri = symbol.as_string();
        if(!new_uri.size() && !uri_.size())
            return 0;
        if(status_ != kStatusStoped)
            graph_->execute_command(kGraphCommandStop);
        ret = open_media(symbol.as_string());
        reset_position();
        graph_->execute_command(kGraphCommandConnect);
        //graph_->cmd_seek(0);
    }
    else if(name == "timeout"){
        timeout_check_ms_ = double(symbol);
    }
    else if(name == "timeout"){
        emit_keyframe_only_ = properties_["emitKeyframeOnly"];
    }
    return ret;
}

int32_t MediaSourceFFmpegFilter::close()
{
    quit_reading_flag_ = true;
    read_condition_.notify_one();
    if(reading_thread_.joinable())
        reading_thread_.join();

    close_context();

    remove_pin(kOutputPin,kPinIndexAll);
    stream_pins_map_.clear();
    bind_pins_to_script(kOutputPin);
    return 0;
}

int32_t MediaSourceFFmpegFilter::close_context()
{
    media_loaded_ = false;
    if(media_file_){
        avformat_close_input(&media_file_);
        media_file_ = nullptr;
    }

    if(custom_io_){
        custom_io_->unuse();
        custom_io_ = nullptr;
    }
    return 0;
}

int32_t MediaSourceFFmpegFilter::do_seek(int32_t ms,int32_t flag)
{
    if (!media_file_ || (media_file_->ctx_flags & AVFMTCTX_UNSEEKABLE))
        return 0;

    if(network_timeout_)
        return -1;
    FFTimestamp timestamp = FFTimestamp(std::chrono::milliseconds(ms));
    int64_t pos = timestamp.rescale().timestamp();

    reset_timeout();
    av_seek_frame(media_file_,-1,pos,flag);
    return 0;
}


int32_t MediaSourceFFmpegFilter::reading_proc()
{
    while (!quit_reading_flag_) {
        pause_wait_.check();//reading lasted frame, better check before wait
        std::unique_lock<std::mutex> lock(read_mutex_);
        read_condition_.wait(lock);

        //for close notify to quit
        if(quit_reading_flag_)
            break;

        if(pause_wait_.check())
            continue;

        int32_t first_pts = -1;
        request_read_to_ = 0;
        while(true){
            if(network_timeout_) {
                MR_LOG_DEAULT("WARNING:Network timeout...");
                //can't call lua functions throw_job_error("timeout");
                Value status((double)kStatusEos);
                set_property_async(kFilterPropertyStatus, status);
                deliver_eos_frame();
                break;
            }

            AVPacket* packet = av_packet_alloc();
            reset_timeout();
            int ret = av_read_frame(media_file_,packet);
            if(ret < 0){
                av_packet_free(&packet);
                if(ret == AVERROR_INVALIDDATA){
                    double seek_back_ms = properties_["seekBackMs"];
                    bool re_connect = properties_["seekBackReConnect"];
                    if(re_connect && reconnect_count_ > 0) {
                        MR_LOG_DEAULT("Reconnectinng net work... {}", reconnect_count_);
                        reconnect_count_--;
                        if (open_media(uri_,true) != 0)
                            continue;
                    } else {
                        MR_ERROR("Reconnect failed, break to end...");
                        break;
                    }
                    skip_read_pts_to_ = current_read_pts_;
                    int32_t seek_to = current_read_pts_ - seek_back_ms;
                    if(seek_to < 100)
                        seek_to = 0;
                    do_seek(seek_to,AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
                    MR_WARN("ffmpeg read failed [AVERROR_INVALIDDATA],re-seek back and re-read [from:{} seek:{} re-connect:{}]",current_read_pts_,seek_to,re_connect);
                    continue;
                }
                MR_LOG_DEAULT("MediaSourceFFmpegFilter::reading_proc() eof");

                Value status((double)kStatusEos);
                set_property_async(kFilterPropertyStatus, status);
                deliver_eos_frame();
                break;
            }

            auto it = stream_pins_map_.find(packet->stream_index);
            if( it == stream_pins_map_.end()){
                av_packet_free(&packet);
                continue;
            }

            AVStream* stream = media_file_->streams[packet->stream_index];

            if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && emit_keyframe_only_ && !(packet->flags & AV_PKT_FLAG_KEY)){
                continue;
            }

            PinPointer& output_pin = it->second;

            int64_t pts = 1000.0 * packet->pts * av_q2d(stream->time_base);
            int64_t dts = 1000.0 * packet->dts * av_q2d(stream->time_base);

            if(skip_read_pts_to_ > 0){
                if(pts == skip_read_pts_to_)
                    skip_read_pts_to_ = 0;
                else
                    MR_LOG_DEAULT("WARNNING:ffmpeg read skip frame at[need:{} read:{}]\n",skip_read_pts_to_,pts);
                continue;
            }
//            MR_WARN(">>>stream {}[{}] read packet pts:{} dts:{} timebase:{}/{}",packet->stream_index,
//                      av_get_media_type_string(stream->codecpar->codec_type),
//                      pts,dts,
//                      stream->time_base.num,stream->time_base.den);
            // A/V PTS maybe large diff, so use max(A/V) to get read-to pts
            if(seeked_){
                //seek maybe back forward, so set current pts when seek
                current_read_pts_ = pts;
                seeked_ = false;
            }

            packet->pts = pts;
            packet->dts = dts;
            current_read_pts_= std::max(current_read_pts_,pts);

            auto new_frame = Frame::make_packet(packet);
            new_frame->releaser = sdmp_frame_free_packet_releaser;

            output_pin->deliver(new_frame);

            // MR_LOG_DEAULT("reading proc: {} {} {}", pts, current_read_pts_, request_read_to_);

            //pts of mp3 pic maybe AV_NOPTS_VALUE
            if(first_pts == -1 && pts >= 0)
                first_pts = pts;

            if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && packet->flags & AV_PKT_FLAG_KEY){
                keyframes_read_++;
                MR_LOG_DEAULT("got a keyframe {}",pts);
            }

            if(properties_["readKeyfarameSection"].as_bool()){
                new_frame->flag = FrameFlags(new_frame->flag | kFrameFlagPreview);
                if(keyframes_read_ == 2){
                    //seek reset to 0 to read key-to-key
                    //linear read reset to 1 to read to next key
                    keyframes_read_ = 1;
                    MR_LOG_DEAULT("READ A WHOLE KEYFRAME SECTION");
                    break;
                }
            }
            else if(current_read_pts_ >= request_read_to_)
                break;

            if(pause_wait_.check())
                break;
        };
    }
    return 0;
}


int32_t MediaSourceFFmpegFilter::reset_timeout()
{
    timeout_check_start_  = MR_TIMER_MS(timeout_timer_);
    return 0;
}

int32_t MediaSourceFFmpegFilter::check_timeout()
{
    int32_t interval = MR_TIMER_MS(timeout_timer_) - timeout_check_start_ ;
    if(interval > timeout_check_ms_){
        MR_ERROR("MediaSourceFFmpegFilter::check_timeout checked a timeout, stop reading media stream!");
        network_timeout_ = true;
        return 1;
    }
    if(quit_reading_flag_){
        MR_WARN("MediaSourceFFmpegFilter::check_timeout checked close reading proc, stop reading media stream!");
        return 1;
    }
    return 0;
}

int32_t MediaSourceFFmpegFilter::reset_position()
{
    current_read_pts_ = -1;    
    skip_read_pts_to_ = -1;
    //seek reset to 0 to read key-to-key
    //linear read reset to 1 to read to next key
    keyframes_read_   = 0;
    if(media_file_){
        if (media_file_->ctx_flags & AVFMTCTX_UNSEEKABLE){
            return 0;
        }
        else
            seeked_           = true;
    }

    return 0;
}

int32_t MediaSourceFFmpegFilter::throw_job_error(const std::string &reson)
{
    auto exception_function = properties_["exceptionHandler"].as<sol::function>();
    if(exception_function.valid()) {
        exception_function(filter_state_,id_, reson);
    }
    return 0;
}

int32_t MediaSourceFFmpegFilter::get_source_media_info(Info &info)
{
    if(!media_file_){
        info = IFilterExtentionMediaCacheSource::Info();
        return -1;
    }
    info.uri_key = uri_;
    info.protocol = properties_["protocol"].as_string();
    info.format_name = media_file_->iformat->name;
    info.duration = properties_["duration"].as_double();

    info.output_pins.clear();
    auto& output_pins = get_pins(kOutputPin);
    for(auto &pin : output_pins)
        info.output_pins.push_back(pin.Get());
    return 0;
}
int32_t MediaSourceFFmpegFilter::active_cache_mode(bool active){
    caching_mode_ = active;
    return 0;
}


}


