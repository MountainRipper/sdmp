#include "media_muxer_ffmpeg.h"

namespace mr::sdmp {

COM_REGISTER_OBJECT(MediaMuxerFFmpegFilter)


MediaMuxerFFmpegFilter::MediaMuxerFFmpegFilter()
{

}


int32_t MediaMuxerFFmpegFilter::initialize(IGraph *graph, const Value &filter_values)
{
    //add a default pin for accept first stream connect
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kInputPin);
    return GeneralFilter::initialize(graph,filter_values);
}

int32_t MediaMuxerFFmpegFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    const auto& formats = sender_pin->formats();
    auto ret = open_muxer();
    if(ret < 0)
        return ret;

    int index = -1;
    for(auto& item : formats){
        ++index;
        int32_t ret = add_stream(item);
        if(ret < 0)
            continue;

        return index;
    }
    return -1;
}

int32_t MediaMuxerFFmpegFilter::connect_chose_output_format(sdmp::IPin *output_pin, int32_t index)
{
    return 0;
}

int32_t MediaMuxerFFmpegFilter::receive(sdmp::IPin *input_pin, std::shared_ptr<sdmp::Frame> frame)
{
    if(status_ != kStatusRunning)
        return 0;

    frame->flag = (FrameFlags)input_pin->index();
    packet_cache_.push(frame);
    return 0;
    if(frame->packet == nullptr)
        return -1;

    auto pin_index = input_pin->index();
    auto it = pin_stream_map_.find(pin_index);
    if(it == pin_stream_map_.end())
        return -2;

    auto stream_index = it->second;
    frame->packet->stream_index = stream_index;

    AVRational time_base = muxer_->streams[stream_index]->time_base;//{ 1, 1000 };
    AVRational rational = {1, 1000};
    if(it->second == 1)
        rational = {1, 48000};

    MR_LOG_DEAULT("muxer write packet: stream:{} size:{} pts:{}",it->second,frame->packet->size,frame->packet->pts);

    frame->packet->pts = av_rescale_q(frame->packet->pts, rational, time_base);
    frame->packet->dts = av_rescale_q(frame->packet->dts, rational, time_base);
    std::unique_lock<std::mutex> lock(write_mutex_);
    return av_interleaved_write_frame(muxer_,frame->packet);
    return 0;
}

int32_t MediaMuxerFFmpegFilter::requare(int32_t duration, const std::vector<sdmp::PinIndex> &output_pins)
{
    //always requaire
    return 1000;
}

int32_t MediaMuxerFFmpegFilter::process_command(const std::string &command, const Value &param)
{
    GeneralFilter::process_command(command,param);
    if(command == kGraphCommandPause){

    }
    else if(command == kGraphCommandStop){
        finish_write_thread();
    }
    else if(command == kGraphCommandPlay){
        start_write_thread();
    }
    return 0;
}

int32_t MediaMuxerFFmpegFilter::open_muxer()
{
    if(muxer_)
        return 0;

    avformat_network_init();

    const char* format_name = nullptr;
    if(properties_["formatName"].as_string().size())
        format_name = properties_["formatName"].as_string().c_str();
    const char* url = properties_["uri"].as_string().c_str();
    {
        //get protocol
        char proto[16];
        av_url_split(proto,16,0,0,0,0,0,0,0,url);
        if(proto[0] == '\0')
            strcpy(proto,"file");
        set_property("protocol",std::string(proto));
        if(std::string(proto) == "rtmp"  ||
            std::string(proto) == "rtmpe"||
            std::string(proto) == "rtmps"||
            std::string(proto) == "rtmpt"||
            std::string(proto) == "rtmpte"||
            std::string(proto) == "rtmpts"){
            format_name = "flv";
        }
    }

    auto ret = avformat_alloc_output_context2(&muxer_,nullptr,format_name,url);
    if(ret < 0)
        return ret;


    return 0;
}

int32_t MediaMuxerFFmpegFilter::add_stream(const Format &format)
{
    AVCodecParameters* params = (AVCodecParameters*)format.codec_parameters;
    if(params == nullptr)
        return -1;

//    AVCodecContext* context = (AVCodecContext*)format.codec_context;
//    if(context == nullptr)
//        return -1;


    if(format.type != AVMEDIA_TYPE_AUDIO && format.type != AVMEDIA_TYPE_VIDEO)
        return -2;

    auto stream = avformat_new_stream(muxer_, nullptr);
    if(stream == nullptr)
        return -3;

    stream->id = muxer_->nb_streams - 1;
    /* Some formats want stream headers to be separate. */
    if (muxer_->oformat->flags & AVFMT_GLOBALHEADER)
        muxer_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    stream->time_base = {format.timebase.numerator,format.timebase.denominator};

    auto ret = avcodec_parameters_copy(stream->codecpar,params);
    if(ret < 0)
        return ret;
    if(stream->time_base.den == 1000)
        stream->time_base.den = 24;

    auto pin_index = get_pins(kInputPin).size()-1;
    sync_update_pin_format(kInputPin,pin_index,0,format);

    pin_stream_map_[pin_index] = stream->id;
    pin_timebase_map_[pin_index] = format.timebase;
    //add a new pin for accept new stream connect
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kInputPin);
    return 0;
}

int32_t MediaMuxerFFmpegFilter::write_header()
{
    if(header_writed_)
        return true;
    int ret = 0;
    const char* uri = properties_["uri"].as_string().c_str();
    if (!(muxer_->flags & AVFMT_NOFILE)) {
        ret = avio_open(&muxer_->pb, uri, AVIO_FLAG_WRITE);
        if (ret < 0) {
            error_ = ret;
            return ret;
        }
    }

    av_dump_format(muxer_, 0, uri, 1);

    ret = avformat_write_header(muxer_, NULL);
    if (ret < 0) {
        return ret;
    }

    header_writed_ = true;
    return 0;
}


int32_t MediaMuxerFFmpegFilter::write_proc()
{
    bool quit_thread_ = false;
    while(!quit_thread_){
        if(!quit_flag_)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::shared_ptr<sdmp::Frame> frame;
        if(packet_cache_.size()){

            if(!header_writed_)
                write_header();

            if(error_ < 0)
                break;
            frame = packet_cache_.pop();
            if(frame->packet == nullptr)
                continue;


            int32_t pin_index = frame->flag;
            auto it = pin_stream_map_.find(pin_index);
            if(it == pin_stream_map_.end())
                continue;

            auto stream_index = it->second;
            frame->packet->stream_index = stream_index;

            if(muxer_->streams[stream_index]->time_base.den == 24)
                muxer_->streams[stream_index]->time_base = {1,1000};

            sdmp::Rational pin_timebase = pin_timebase_map_[pin_index];
            AVRational stream_timebase = muxer_->streams[stream_index]->time_base;
            AVRational frame_timebase  = {pin_timebase.numerator, pin_timebase.denominator};


            frame->packet->pts = av_rescale_q(frame->packet->pts, frame_timebase, stream_timebase);
            frame->packet->dts = av_rescale_q(frame->packet->dts, frame_timebase, stream_timebase);
            //MR_LOG_DEAULT("muxer write packet: stream:{} size:{} pts:{}",it->second,frame->packet->size,frame->packet->pts);
            av_interleaved_write_frame(muxer_,frame->packet);
        }
        else if(quit_flag_)
            quit_thread_ = true;
    }

    if(!error_ && muxer_){
        av_write_trailer(muxer_);
        if (!(muxer_->oformat->flags & AVFMT_NOFILE))
            avio_closep(&muxer_->pb);
        avformat_free_context(muxer_);
    }
    muxer_ = nullptr;
    error_ = 0;
    return 0;
}

int32_t MediaMuxerFFmpegFilter::start_write_thread()
{
    quit_flag_ = false;
    if(write_thread_.joinable() == false)
        write_thread_ = std::thread(&MediaMuxerFFmpegFilter::write_proc,this);
    return 0;
}

int32_t MediaMuxerFFmpegFilter::finish_write_thread()
{
    quit_flag_ = true;
    if(write_thread_.joinable())
        write_thread_.join();
    header_writed_ = false;
    if(packet_cache_.size()){
        MR_LOG_DEAULT("WARNNING: MediaMuxerFFmpegFilter::finish_write_thread not all packets writed!");
        packet_cache_.clear();
    }
    return 0;
}

}


