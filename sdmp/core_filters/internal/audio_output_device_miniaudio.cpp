#define MINIAUDIO_IMPLEMENTATION
#include "audio_output_device_miniaudio.h"
#include "audio_mixer_input_pin.h"
#include <logger.h>

namespace mr::sdmp {

void on_log(void* pUserData, ma_uint32 level, const char* message)
{
    MP_LOG_DEAULT("{}: {}", ma_log_level_to_string(level), message);
}

void on_data(ma_device* pDevice, void* pFramesOut, const void* pFramesIn, ma_uint32 frameCount)
{
    sdmp::AudioOutputDeviceMiniaudioFilter* owner = (sdmp::AudioOutputDeviceMiniaudioFilter*)pDevice->pUserData;
    owner->on_playback(pFramesOut,frameCount);
}

void on_stop(ma_device* pDevice)
{
    (void)pDevice;
}


ma_bool32 try_parse_backend(const char* arg, ma_backend* pBackends)
{
    *pBackends = ma_backend_null;
    if (strcmp(arg, "wasapi") == 0) {
        *pBackends = ma_backend_wasapi;
        goto done;
    }
    if (strcmp(arg, "dsound") == 0) {
        *pBackends = ma_backend_dsound;
        goto done;
    }
    if (strcmp(arg, "winmm") == 0) {
        *pBackends = ma_backend_winmm;
        goto done;
    }
    if (strcmp(arg, "coreaudio") == 0) {
        *pBackends = ma_backend_coreaudio;
        goto done;
    }
    if (strcmp(arg, "sndio") == 0) {
        *pBackends = ma_backend_sndio;
        goto done;
    }
    if (strcmp(arg, "audio4") == 0) {
        *pBackends = ma_backend_audio4;
        goto done;
    }
    if (strcmp(arg, "oss") == 0) {
        *pBackends = ma_backend_oss;
        goto done;
    }
    if (strcmp(arg, "pulseaudio") == 0) {
        *pBackends = ma_backend_pulseaudio;
        goto done;
    }
    if (strcmp(arg, "alsa") == 0) {
        *pBackends = ma_backend_alsa;
        goto done;
    }
    if (strcmp(arg, "jack") == 0) {
        *pBackends = ma_backend_jack;
        goto done;
    }
    if (strcmp(arg, "aaudio") == 0) {
        *pBackends = ma_backend_aaudio;
        goto done;
    }
    if (strcmp(arg, "opensl") == 0) {
        *pBackends = ma_backend_opensl;
        goto done;
    }
    if (strcmp(arg, "webaudio") == 0) {
        *pBackends = ma_backend_webaudio;
        goto done;
    }
    if (strcmp(arg, "null") == 0) {
        *pBackends = ma_backend_null;
        goto done;
    }

    return false;
done:
    return true;
}

COM_REGISTER_OBJECT(AudioOutputDeviceMiniaudioFilter)

AudioOutputDeviceMiniaudioFilter::AudioOutputDeviceMiniaudioFilter()
    :GeneralFilter(CLSID_AudioOutputDeviceMiniaudioFilter)
{
    memset(&pull_ring_buffer_,0,sizeof (pull_ring_buffer_));
}

AudioOutputDeviceMiniaudioFilter::~AudioOutputDeviceMiniaudioFilter()
{
    if(device_){
        ma_device_stop(device_.get());
        ma_device_uninit(device_.get());
    }
    if(pull_ring_buffer_.rb.pBuffer){
        ma_pcm_rb_uninit(&pull_ring_buffer_);
    }
    //static resource release in c++ runtime, logger_xxx call spdlog::set_pattern will crash
    MP_INFO("MiniAudio Stopped");
}

int32_t AudioOutputDeviceMiniaudioFilter::initialize(sdmp::IGraph *graph, const Value &filter_values)
{
    //NOTE, do not call GeneralFilter::init because it not in read graph
    if(GeneralFilter::initialize(graph,filter_values)){
        return kErrorInvalidParameter;
    }
    auto config = filter_values.as<sol::table>();

    std::string selector = config["selector"].get_or(std::string(""));
    std::string backend = config["backend"].get_or(std::string(""));
    int32_t samplerate = config["samplerate"].get_or(48000);
    int32_t channels = config["channels"].get_or(2);
    int32_t framesize = config["framesize"].get_or(480);
    int32_t bits = config["bits"].get_or(32);

    auto contextConfig = ma_context_config_init();
    ma_log_init(NULL,&ma_log_);
    ma_log_register_callback(&ma_log_,ma_log_callback_init(on_log,(void*)this));
    contextConfig.pLog = &ma_log_;

    ma_backend chose_backend[1] = {ma_backend_null};
    auto get = try_parse_backend(backend.c_str(),chose_backend);

    context_ = std::shared_ptr<ma_context>(new ma_context());
    auto result = ma_context_init(get?chose_backend:nullptr, 1, &contextConfig, context_.get());
    MP_LOG_DEAULT("select backend: {} {} {}", backend, (int)chose_backend[0], (void*)this);

    int32_t select_index = -1;
    ma_device_info* playback_devices_info;
    ma_uint32 playback_devices_count;
    if(selector.size()){
        MP_LOG_DEAULT("--------------------------------------opening selected device: {}", selector.data());
        device_id_ = std::shared_ptr<BufferObject>(new BufferObject(sizeof (ma_device_id)));
        result = ma_context_get_devices(context_.get(), &playback_devices_info, &playback_devices_count, nullptr, nullptr);
        if (result != MA_SUCCESS) {
            MP_LOG_DEAULT("---------------------------------- Failed to retrieve device information.");
            return -3;
        }
        MP_LOG_DEAULT("Playback Devices");
        for (int index = 0; index < playback_devices_count; ++index) {
            MP_LOG_DEAULT("    {}: {} {}", index, playback_devices_info[index].name,playback_devices_info[index].id.alsa);
            std::string name = playback_devices_info[index].name;
            if(name.find(selector) !=  std::string::npos){
                select_index = index;
            }
        }
    }

    MP_LOG_DEAULT("---------------------- selector index: {}", select_index);

    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    if(select_index >= 0)
        deviceConfig.playback.pDeviceID = &playback_devices_info[select_index].id;
    deviceConfig.playback.format   = (bits == 32) ? ma_format_f32 : ma_format_s16;
    deviceConfig.playback.channels = channels;
    deviceConfig.periodSizeInFrames = framesize;
    deviceConfig.sampleRate        = samplerate;
    deviceConfig.dataCallback      = on_data;
    deviceConfig.stopCallback      = on_stop;

    device_ = std::shared_ptr<ma_device>(new ma_device());
    result = ma_device_init(context_.get(), &deviceConfig, device_.get());
    if (result != MA_SUCCESS) {
        MP_LOG_DEAULT("Failed to initialize device.");
        ma_context_uninit(context_.get());
        return -1;
    }

    format_     = deviceConfig.playback.format;
    channels_   = deviceConfig.playback.channels;
    samplerate_ = deviceConfig.sampleRate;
    frame_size_ = deviceConfig.periodSizeInFrames;
    ma_pcm_rb_init(format_,channels_, frame_size_*16, NULL, NULL, &pull_ring_buffer_);

    Format format;
    format.type = AVMEDIA_TYPE_AUDIO;
    format.codec = (bits == 32) ? AV_CODEC_ID_PCM_F32LE : AV_CODEC_ID_PCM_S16LE;
    format.format =(bits == 32) ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16;
    format.samplerate = samplerate;
    format.channels = channels;
    format.timebase = {1,samplerate_};

    av_params_.frame_size = framesize;
    format.codec_parameters = &av_params_;

    int bytes_per_second = 0;
    av_samples_get_buffer_size(&bytes_per_second,format.channels,format.samplerate,(AVSampleFormat)format.format,1);
    format.bitrate = bytes_per_second*8;

    pin_.CoCreateInstanceDirectly<AudioMixerInputPin>();

    FilterPointer filter= FilterPointer(static_cast<IFilter*>(this));
    std::vector<Format> formats = {format};
    pin_->initialize(filter,formats,kInputPin);

    add_pin(pin_);

    device_->pUserData = this;


    MP_LOG_DEAULT("AudioOutputMiniaudio::init Device: [{}][{}]",ma_get_backend_name(context_->backend), device_->playback.name);
    return 0;
}

int32_t AudioOutputDeviceMiniaudioFilter::process_command(const std::string &command, const Value& param)
{
    MP_LOG_DEAULT("AudioOutputDeviceMiniaudioFilter::process_command {}", command, (void*)device_.get());
    (void)param;
    if(!device_)
        return -1;

    auto stated = ma_device_is_started(device_.get());
    int result = 0;
    if(command == kGraphCommandPlay && !stated){
        result = ma_device_start(device_.get());
        if (result != MA_SUCCESS){
            MP_LOG_DEAULT("Failed to start device.");
        }else
            MP_LOG_DEAULT("audio device {} started  {}", device_->playback.name, (void*)this);
    }
    else if(command == kGraphCommandStop && stated){
        result = ma_device_stop(device_.get());
        if (result != MA_SUCCESS){
            MP_LOG_DEAULT("Failed to start device.","");
        }
        else
            MP_LOG_DEAULT("audio device {} stoped ", device_->playback.name);
    }
    return 0;
}

int32_t AudioOutputDeviceMiniaudioFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    return 0;
}

int32_t AudioOutputDeviceMiniaudioFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t AudioOutputDeviceMiniaudioFilter::receive(IPin* input_pin,FramePointer frame)
{
    return 0;
}

int32_t AudioOutputDeviceMiniaudioFilter::get_property(const std::string& property,Value& value)
{
    if(property == kFilterPropertyDevice){
        value = std::string(device_->playback.name);
    }
    return -1;
}

int32_t AudioOutputDeviceMiniaudioFilter::requare(int32_t duration,const std::vector<PinIndex>& output_pins)
{
    return 0;
}

int32_t AudioOutputDeviceMiniaudioFilter::master_loop(bool before_after)
{
    return 0;
}


int32_t AudioOutputDeviceMiniaudioFilter::on_playback(void *pcm, int32_t frames)
{    
    ma_uint32 pcmFramesAvailableInRB;
    ma_uint32 pcmFramesProcessed = 0;
    ma_uint8* pRunningOutput = (ma_uint8*)pcm;

    /*
    The first thing to do is check if there's enough data available in the ring buffer. If so we can read from it. Otherwise we need to keep filling
    the ring buffer until there's enough, making sure we only fill the ring buffer in chunks of PCM_FRAME_CHUNK_SIZE.
    */
    while (pcmFramesProcessed < frames) {    /* Keep going until we've filled the output buffer. */
        ma_uint32 framesRemaining = frames - pcmFramesProcessed;

        pcmFramesAvailableInRB = ma_pcm_rb_available_read(&pull_ring_buffer_);
        if (pcmFramesAvailableInRB > 0) {
            ma_uint32 framesToRead = (framesRemaining < pcmFramesAvailableInRB) ? framesRemaining : pcmFramesAvailableInRB;
            void* pReadBuffer;

            ma_pcm_rb_acquire_read(&pull_ring_buffer_, &framesToRead, &pReadBuffer);
            {
                memcpy(pRunningOutput, pReadBuffer, framesToRead * ma_get_bytes_per_frame(format_, channels_));
            }
            ma_pcm_rb_commit_read(&pull_ring_buffer_, framesToRead);

            //MP_LOG_DEAULT("AudioOutputMiniaudio::on_playback: read RB {}",framesToRead);

            pRunningOutput += framesToRead * ma_get_bytes_per_frame(format_, channels_);
            pcmFramesProcessed += framesToRead;
        } else {
            /*
            There's nothing in the buffer. Fill it with more data from the callback. We reset the buffer first so that the read and write pointers
            are reset back to the start so we can fill the ring buffer in chunks of PCM_FRAME_CHUNK_SIZE which is what we initialized it with. Note
            that this is not how you would want to do it in a multi-threaded environment. In this case you would want to seek the write pointer
            forward via the producer thread and the read pointer forward via the consumer thread (this thread).
            */
            ma_uint32 framesToWrite = frame_size_;
            void* pWriteBuffer;

            ma_pcm_rb_reset(&pull_ring_buffer_);
            ma_pcm_rb_acquire_write(&pull_ring_buffer_, &framesToWrite, &pWriteBuffer);
            {
                MA_ASSERT(framesToWrite == frame_size_);   /* <-- This should always work in this example because we just reset the ring buffer. */
                //data_callback_fixed(pDevice, pWriteBuffer, NULL, framesToWrite);

                pin_requare_data(pWriteBuffer,frame_size_);
                //MP_LOG_DEAULT("AudioOutputMiniaudio::on_playback: Request RB {}",frame_size_);
            }
            ma_pcm_rb_commit_write(&pull_ring_buffer_, framesToWrite);
        }
    }
    return 0;
}

int32_t AudioOutputDeviceMiniaudioFilter::pin_requare_data(void *pcm, int32_t frames)
{
    AudioMixerInputPin* mixer_pin = static_cast<AudioMixerInputPin*>(pin_.Get());
    mixer_pin->requare(pcm,frames);
    return 0;
}


}
