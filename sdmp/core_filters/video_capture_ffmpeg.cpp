#include "video_capture_ffmpeg.h"
#include "core_includes.h"

namespace mr::sdmp {

COM_REGISTER_OBJECT(VideoCaptureFFmpegFilter)

VideoCaptureFFmpegFilter::VideoCaptureFFmpegFilter()
{

}

int32_t VideoCaptureFFmpegFilter::initialize(IGraph *graph, const Value &config_value)
{
    GeneralFilter::initialize(graph,config_value);
    return 0;
}

int32_t VideoCaptureFFmpegFilter::process_command(const std::string &command, const Value& param)
{
    return GeneralFilter::process_command(command,param);
}

int32_t VideoCaptureFFmpegFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    return 0;
}

int32_t VideoCaptureFFmpegFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{    
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t VideoCaptureFFmpegFilter::receive(IPin* input_pin,FramePointer frame)
{
    return 0;
}



int32_t VideoCaptureFFmpegFilter::requare(int32_t duration,const std::vector<PinIndex>& output_pins)
{
    return 0;
}

int32_t VideoCaptureFFmpegFilter::property_changed(const std::string& name,Value& symbol)
{
    return 0;
}

int32_t VideoCaptureFFmpegFilter::open_device()
{
    AVFormatContext	*format_context = avformat_alloc_context();
    AVCodecContext	*codec_context = nullptr;
    const AVCodec	*codec = nullptr;
    int video_stream_index = -1;

    AVDictionary* dict = nullptr;
    av_dict_set(&dict,"video_size",properties_["size"].as_string().c_str(),0);
    av_dict_set(&dict,"pixel_format",properties_["format"].as_string().c_str(),0);
    const AVInputFormat *ifmt=av_find_input_format("v4l2");

    int err = 0;
    do{
        if(avformat_open_input(&format_context,properties_["device"].as_string().c_str(),ifmt,&dict)!=0){
            MP_LOG_DEAULT("Couldn't open input stream.");
        }
        if(format_context == nullptr){
            err = -1;
            break;
        }

        if(format_context->nb_streams == 0)
        {
            MP_LOG_DEAULT("Couldn't find stream information.");
            err = -2;
            break;
        }


        for(int index=0; index<format_context->nb_streams; index++){
            if(format_context->streams[index]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
            {
                video_stream_index = index;
//                m_inputSize = QSize(format_context->streams[index]->codec->width,format_context->streams[index]->codec->height);
//                setOutputSize(QSize(int(m_inputSize.width() * m_scale) / 2 * 2,
//                                    int(m_inputSize.height()* m_scale) / 2 * 2));
//                qInfo()<<"AimyCameraCapture capture:"<<m_inputSize<<" output:"<<m_outputSize;
                break;
            }
        }
        if(video_stream_index==-1)
        {
            MP_LOG_DEAULT("Couldn't find a video stream.");
            err = -2;
            break;
        }

        codec_context = avcodec_alloc_context3(nullptr);
        if (!codec_context) {
            MP_LOG_DEAULT("Could not allocate AVCodecContext.");
            err = -3;
            break;
        }
        if (avcodec_parameters_to_context(codec_context, format_context->streams[video_stream_index]->codecpar) < 0) {
            MP_LOG_DEAULT("avcodec_parameters_to_context() failed.");
            err = -3;
            break;
        }

        codec=avcodec_find_decoder(codec_context->codec_id);
        if(codec==NULL)
        {
            MP_LOG_DEAULT("Codec not found.");
            err = -3;
            break;
        }
        if(avcodec_open2(codec_context, codec,NULL)<0)
        {
            MP_LOG_DEAULT("Could not open codec.");
            err = -4;
            break;
        }
    }while (false);

    if(err < 0){
        return err;
    }
    return 0;
}

}


