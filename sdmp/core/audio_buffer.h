#ifndef AV_ENGINE_COMMON_PCM_BUFFER_H_
#define AV_ENGINE_COMMON_PCM_BUFFER_H_
#include <list>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <memory>
#include <string.h>
using namespace std;

namespace sdp {

static const int32_t kEndOfBuffer   = -100;
static const int32_t kErrorOutRange = -101;
static const int32_t kSeekHeadBuffer = -102;

class AudioCacheFrame {
public:
  AudioCacheFrame() {
    memset(this, 0, sizeof(*this));
  }
  ~AudioCacheFrame(void) {
    if (NULL != data_) {
      free(data_);
    }
  }

  long Create(uint32_t channel, uint32_t samplerate,uint32_t sample_bytes, const void* data, uint32_t bytes, double pts, uint64_t stream_number) {
    if (0 == channel || 0 == samplerate || 0 == bytes || NULL == data)
    {
      return -1;
    }
    channels_ = channel;
    samplerate_ = samplerate;
    size_in_bytes_ = bytes;
    pts_ = pts;
    stream_number_ = stream_number;

    sample_count_ = bytes  / (channel * sample_bytes);

    duration_millesecond_ = sample_count_;
    duration_millesecond_ = duration_millesecond_ * 1000 / samplerate;

    if(NULL != data_)
    {
      free(data_);
    }

    data_ = new uint8_t[size_in_bytes_];
    memcpy(data_, data, size_in_bytes_);
    return 0;
  }

 
public:
  uint32_t	channels_;
  uint32_t	samplerate_;
  uint32_t	sample_count_;
  uint32_t	size_in_bytes_;
  double	duration_millesecond_;
  double	pts_;
  uint8_t*	data_;
  uint64_t      stream_number_;
};


class RemoveAudioFrameOperator{
public:
    RemoveAudioFrameOperator(uint64_t stream_number){
        stream_number_ = stream_number;
    }

    bool operator ()(std::shared_ptr<AudioCacheFrame>& frame)
    {
        return frame->stream_number_ == stream_number_;
    }
private:
    uint64_t stream_number_ = 0;
};

class AudioBuffer {
public:
  AudioBuffer() {
      samplerate_ = 0;
      channels_ = 0;
      sample_read_position_ = 0;
      bytes_per_channels_sample_ = 0;
      samples_per_millesecond_ = 0;
      totle_sample_count_ = 0;
      duration_ = 0;
      format_ = 0;
      sample_format_bytes_ = 2;
  }
  ~AudioBuffer(void) {
      clear();
  }
public:

  uint32_t reset(uint32_t samplerate, uint8_t channles,uint8_t sample_bytes) {

      if (0 == samplerate || 0 == channles) {
        return -1;
      }
      samplerate_ = samplerate;
      channels_ = channles;
      bytes_per_channels_sample_ = channles * sample_bytes;
      sample_format_bytes_ = sample_bytes;
      samples_per_millesecond_ = static_cast<uint8_t>(samplerate / 1000);
      duration_ = 0;
      return clear();
  }

  uint32_t push_pcm(const void* pcm, uint32_t bytes, double pts, uint64_t stream_number) {

      if(pcm == nullptr || bytes == 0)
          return -1;

      std::shared_ptr<AudioCacheFrame> item = std::shared_ptr<AudioCacheFrame>(new AudioCacheFrame());

      item->Create(channels_, samplerate_,sample_format_bytes_, pcm, bytes, pts, stream_number);
      packets_.push_back(item);
      totle_sample_count_ += item->sample_count_;
      duration_ = (uint32_t)(totle_sample_count_ * 1000.0 / samplerate_);
      return item->sample_count_;
  }

  uint32_t push_frame(std::shared_ptr<AudioCacheFrame> item) {
      packets_.push_back(item);
      totle_sample_count_ += item->sample_count_;
      duration_ = (uint32_t)(totle_sample_count_ * 1000.0 / samplerate_);
      return item->sample_count_;
  }

  uint32_t pull(uint32_t sample_count, uint8_t* pcm) {

      if (0 == sample_count) {
        return -1;
      }
      if (packets_.empty()) {
         memset(pcm,0,sample_count*bytes_per_channels_sample_);
         return 0;
      }
      uint32_t sample_count_need = sample_count;
      do {
        auto frame = packets_.front();
        uint32_t valid_sample_count = frame->sample_count_ - sample_read_position_;
        if (sample_count_need < valid_sample_count) {

          uint32_t read_bytes = sample_count_need * bytes_per_channels_sample_;
          uint8_t* read_point = frame->data_ + sample_read_position_ * bytes_per_channels_sample_;
          if(pcm != nullptr)
          {
              memcpy(pcm, read_point , read_bytes);
          }
          sample_read_position_ += sample_count_need;
          totle_sample_count_ -= sample_count_need;
          sample_count_need -= sample_count_need;
        }
        else {

          uint8_t* read_point = frame->data_ + sample_read_position_ * bytes_per_channels_sample_;
          uint32_t valid_bytes = valid_sample_count * bytes_per_channels_sample_;
          if(pcm != nullptr)
          {
              memcpy(pcm, read_point , valid_bytes);
              pcm += valid_bytes;
          }
          packets_.pop_front();
          sample_read_position_ = 0;
          sample_count_need -= valid_sample_count;
          totle_sample_count_ -= valid_sample_count;
        }
      }
      while (0 != sample_count_need && 0 != packets_.size());
      duration_ = (uint32_t)(totle_sample_count_ * 1000.0 / samplerate_);
      return sample_count - sample_count_need;
  }

  uint32_t clear() {

      sample_read_position_ = 0;
      packets_.clear();
      duration_ = 0;
      totle_sample_count_ = 0;
      sample_read_position_ = 0;
      return 0;
  }

  uint32_t clear_sample_list(){
      packets_.clear();
      return 0;
  }

  uint32_t duration() {
      return duration_;
  }

  uint32_t frames_count() {
      return (uint32_t)packets_.size();
  }

  uint32_t samplerate() {
      return samplerate_;
  }

  uint32_t channels() {
      return channels_;
  }

  double head_pts() {
      if (packets_.empty()) {
        return -1.0;
      }
      if (0 == samples_per_millesecond_) {
        return -1.0;
      }
      auto frame = packets_.front();
      double duration_has_read = sample_read_position_;
      duration_has_read = duration_has_read / samples_per_millesecond_;
      return frame->pts_ + duration_has_read;
  }

  uint64_t head_stream_number() {
      if (packets_.empty()) {
        return 0;
      }
      auto frame = packets_.front();
      return frame->stream_number_;
  }

  uint32_t remove_tail() {

      if (packets_.empty()) {
        return 0;
      }
      if (packets_.size() == 1) {
        clear();
        return 0;
      }

      auto frame = packets_.back();
      packets_.pop_back();

      totle_sample_count_ -= frame->sample_count_;

      if (0 == samplerate_ || 0 == totle_sample_count_) {
        return 0;
      }
      double dur = totle_sample_count_;
      dur = dur / samplerate_;
      return static_cast<uint32_t>(dur * 1000);
  }

  uint32_t samples_count()
  {
      return totle_sample_count_;
  }

  uint32_t channels_sample_bytes()
  {
      return bytes_per_channels_sample_;
  }

  uint32_t sample_bytes()
  {
      return sample_format_bytes_;
  }

  list<std::shared_ptr<AudioCacheFrame>>& packets()
  {
      return packets_;
  }

  virtual int32_t remove_stream(uint64_t stream_number){
      //duration and sample count NOT re-calc
      packets_.remove_if(RemoveAudioFrameOperator(stream_number));
      return 0;
  }

protected:

  uint32_t		samplerate_;

  uint8_t		channels_;

  uint32_t		sample_read_position_;

  uint32_t		bytes_per_channels_sample_;//totle bytes per totle channels sample

  uint32_t		sample_format_bytes_;//bytes per sample uint

  uint8_t		samples_per_millesecond_;

  list<std::shared_ptr<AudioCacheFrame>> packets_;

  uint32_t		totle_sample_count_;

  uint32_t		duration_;

  uint32_t      format_;
};

class AudioBufferPullable : public AudioBuffer
{
public:
    virtual int32_t prepair_buffer(){
        end_ms_ = duration_fixed_;
        return 0;
    }

    virtual int32_t clear_buffer(){
        start_ms_ = 0;
        end_ms_   = 0;
        start_position_ = 0;
        end_position_   = 0;
        cycle_pull_     = false;
        limited_duration_ = 0;
        return 0;
    }

    virtual int32_t set_range(uint32_t start_ms,uint32_t end_ms){
        if(end_ms > duration_fixed_)
            end_ms = duration_fixed_;

        if(end_ms <= start_ms)
            return -1;

        start_ms_ = start_ms;
        end_ms_ = end_ms;

        start_position_ = (uint64_t)ceil((double)start_ms_ * samplerate_ / 1000);// start samples positon
        end_position_   = (uint64_t)floor((double)end_ms_   * samplerate_ / 1000);// end sample positon
        return 0;
    }

    virtual int32_t set_header_duration(int32_t header_duration){
        header_duration_ = header_duration;
        header_sample_position_ = 0;
        header_samples_  = samplerate_ * (header_duration / 1000.0);
        return 0;
    }

    virtual int32_t duration_logic(){
        if(cycle_pull_ == false)
            return int32_t(end_ms_ - start_ms_ + header_duration_);
        else if(limited_duration_ > 0)
            return (int32_t)limited_duration_;
        return INT32_MAX;
    }

    virtual int32_t duration_ranged(){
        return int32_t(end_ms_ - start_ms_);
    }

    virtual int32_t duration_fixed(){
        return duration_fixed_;
    }

    virtual uint64_t header_duration(){
        return header_duration_;
    }

    virtual uint64_t duration_limited(){
        return limited_duration_;
    }

    virtual uint32_t current_logic_position(){
        return (uint32_t)logic_pull_position_;
    }

    virtual int32_t seek_to_head(bool fully = true)
    {
        if(fully){
            logic_pull_position_    = 0;
            header_sample_position_ = 0;
        }
        return 0;
    }

    virtual int32_t seek_to_position(uint64_t position_ms){
        if(limited_duration_ > 0){
            if(position_ms > limited_duration_)
                position_ms = limited_duration_;
        }

        uint64_t position_in_flat = position_ms;
        if(header_duration_ > 0){
            position_in_flat = 0;
            if(position_ms > header_duration_)
                position_in_flat = position_ms - header_duration_;
            header_sample_position_ = (uint64_t)std::min(position_ms,header_duration_) / 1000.0 * samplerate_;
        }

        logic_pull_position_ = position_ms;

        if(cycle_pull_){
            //get cycle position
            position_in_flat = position_in_flat % (std::min(end_ms_ ,(uint64_t)duration_fixed_) - start_ms_);
        }
        else if(position_in_flat > (end_ms_ - start_ms_)){
            //out of range
            logic_pull_position_       = end_position_;
            return kErrorOutRange;
        }
        return (int32_t)(position_in_flat + start_ms_);
    }



    virtual int32_t enable_cycle_pull(bool enable){
        cycle_pull_ = enable;
        return 0;
    }

    virtual int32_t set_limited_duration(int32_t limited_duration){
        limited_duration_ = limited_duration;
        return 0;
    }

    virtual uint64_t start_ms(){
        return start_ms_;
    }

    virtual uint64_t end_ms(){
        return end_ms_;
    }

    virtual bool cycle_pull(){
        return cycle_pull_;
    }


    virtual int32_t pull_pcm(int32_t sample_count, uint8_t* pcm,int32_t* status = nullptr){
        memset(pcm,0,sample_count*bytes_per_channels_sample_);

        if(status)
           * status = 0;

        if(limited_duration_ > 0){
            if(logic_pull_position_ > limited_duration_){
                if(status)
                    *status = kErrorOutRange;
                return kErrorOutRange;
            }
        }

        if(header_samples_ > 0 && header_sample_position_ < header_samples_){
            header_sample_position_ += sample_count;
            if(header_sample_position_ > header_samples_)
            {
                sample_count = header_sample_position_ - header_samples_;
                header_sample_position_ = header_samples_;
            }
            else{
                //in header range
                //return sample_count;
            }
        }
        else
            sample_count = 0;


        return sample_count;
    }
protected:
    double                   logic_pull_position_       = 0;
    uint64_t                 duration_fixed_            = 0;
    uint64_t                 start_ms_                  = 0;
    uint64_t                 end_ms_                    = 0;
    uint64_t                 start_position_            = 0;
    uint64_t                 end_position_              = 0;
    uint64_t                 header_duration_           = 0;
    uint64_t                 header_samples_            = 0;
    uint64_t                 header_sample_position_    = 0;
    bool                     cycle_pull_                = false;
    uint64_t                 limited_duration_          = 0;
};

class PcmBufferFlat : public AudioBufferPullable
{
public:
    ~PcmBufferFlat()
    {
        clear_buffer();
    }

    virtual int32_t prepair_buffer()
    {
        if(packets_.empty())
            return -1;

        flat_buffer_bytes_ = (totle_sample_count_ + packets_.front()->sample_count_) * bytes_per_channels_sample_;
        flat_buffer_ = std::shared_ptr<uint8_t>(new uint8_t[flat_buffer_bytes_],std::default_delete<uint8_t[]>());
        uint8_t* buffer = flat_buffer_.get();
        for(auto frame : packets_)
        {
            memcpy(buffer,frame->data_,frame->size_in_bytes_);
            buffer += frame->size_in_bytes_;
        }
        flat_buffer_read_position_ = 0;
        start_position_ = 0;
        end_position_ = totle_sample_count_;
        start_ms_ = 0;
        end_ms_ = duration_;
        duration_fixed_ = duration_;
        AudioBufferPullable::prepair_buffer();
        return 0;
    }

    virtual int32_t clear_buffer(){
        flat_buffer_ = nullptr;
        flat_buffer_bytes_ = 0;
        flat_buffer_read_position_ = 0;
        AudioBufferPullable::clear_buffer();
        return 0;
    }

    virtual int32_t set_range(uint32_t start_ms,uint32_t end_ms){
        AudioBufferPullable::set_range(start_ms,end_ms);
        flat_buffer_read_position_ = start_position_;
        return 0;
    }

    virtual int32_t pull_pcm(int32_t sample_count, uint8_t* pcm,int32_t* status = nullptr)
    {
        int32_t got_samples = AudioBufferPullable::pull_pcm(sample_count,pcm,status);
        if(got_samples < 0)
            return got_samples;

        if(sample_count == got_samples){
            logic_pull_position_ += sample_count * 1000.0 / samplerate_;
            return got_samples;
        }

        sample_count -= got_samples;

        int32_t samples_tail = end_position_ - flat_buffer_read_position_;
        if(samples_tail == 0){
            if(status)
                *status = kEndOfBuffer;
            if(cycle_pull_){
                seek_to_head(false);
            }
            else
                return kEndOfBuffer;
        }
        if(samples_tail < sample_count)
            sample_count = samples_tail;


        uint64_t read_position = flat_buffer_read_position_ * bytes_per_channels_sample_;
        uint64_t read_bytes    = sample_count * bytes_per_channels_sample_;

        memcpy(pcm,flat_buffer_.get() + read_position ,read_bytes);

        flat_buffer_read_position_ += sample_count;
        logic_pull_position_       += sample_count * 1000.0 / samplerate_;
        return sample_count;
    }

    virtual int32_t seek_to_head(bool fully = true)
    {
        flat_buffer_read_position_ = start_position_;
        AudioBufferPullable::seek_to_head(fully);
        return 0;
    }

    virtual int32_t seek_to_position(uint64_t position_ms)
    {
        int32_t position_in_flat = AudioBufferPullable::seek_to_position(position_ms);
        if(position_in_flat == kErrorOutRange)
        {
            flat_buffer_read_position_ = end_position_;
            return -1;
        }
        //seek to flat position
        flat_buffer_read_position_ = start_position_ + ((uint64_t)position_in_flat * samplerate_ / 1000);
        return 0;
    }

    uint32_t current_flat_position_ms(){
        if(duration_fixed_ == 0)
            return 0;
        double pos = flat_buffer_read_position_ * 1000.0 / samplerate_ - start_ms_;
        return (uint32_t)pos;
    }

    std::shared_ptr<uint8_t> buffer(){
        return flat_buffer_;
    }

    size_t buffer_size(){
        return flat_buffer_bytes_;
    }


private:
    std::shared_ptr<uint8_t> flat_buffer_;
    size_t                   flat_buffer_bytes_;
    uint64_t                 flat_buffer_read_position_ = 0;
};

}

#endif /*AV_ENGINE_COMMON_PCM_BUFFER_H_*/
