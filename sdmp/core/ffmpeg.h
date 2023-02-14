#ifndef FFMPEG_H
#define FFMPEG_H


extern "C"
{

#include <stdint.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/common.h>
#include <libavutil/intreadwrite.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/jni.h>

#include <libavformat/avformat.h>

#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <libavdevice/avdevice.h>
}

class FFRational{
public:
    FFRational(){
        rational_.den = 0;
        rational_.num = 0;
    }
    FFRational(const AVRational& rational)
        :rational_(rational){

    }
    FFRational(const FFRational& other)
        :rational_(other.rational_){

    }
    FFRational(int64_t num, int64_t den){
        rational_.den = den;
        rational_.num = num;
    }

    FFRational& operator = (const AVRational& rational){
        rational_ = rational;
        return *this;
    }

    FFRational&  operator =(const FFRational& other){
        rational_ = other.rational_;
        return *this;
    }

    double as_double(){
        return (double)rational_.num / (double)rational_.den;
    }

    const AVRational& value() const{
        return rational_;
    }
    void set_value(const AVRational& rational){
        rational_ = rational;
    }

    int64_t num(){
        return rational_.num;
    }
    void set_num(int64_t value){
        rational_.num = value;
    }

    int64_t den(){
        return rational_.den;
    }
    void set_den(int64_t value){
        rational_.den = value;
    }

    int64_t rescale(int64_t this_value,const FFRational& dest_rational){
        return av_rescale_q(this_value,rational_,dest_rational.value());
    }
private:
    AVRational rational_;
};

#define FFTimeBaseQ FFRational(1,AV_TIME_BASE)


class FFTimestamp{
public:
    FFTimestamp()
        :timestamp_(AV_NOPTS_VALUE){

    }
    FFTimestamp(int64_t timestamp, const FFRational &rational=FFTimeBaseQ)
        :timestamp_(timestamp)
        ,rational_(rational){

    }
    FFTimestamp(const FFTimestamp& other)
        :timestamp_(other.timestamp_)
        ,rational_(other.rational_){

    }
    template<typename Duration, typename = typename Duration::period>
    FFTimestamp(const Duration& duration)
    {
        using Ratio = typename Duration::period;

        static_assert(Ratio::num <= INT_MAX, "To prevent precision lost, ratio numerator must be less then INT_MAX");
        static_assert(Ratio::den <= INT_MAX, "To prevent precision lost, ratio denominator must be less then INT_MAX");

        timestamp_ = duration.count();
        rational_  = FFRational(static_cast<int>(Ratio::num),
                               static_cast<int>(Ratio::den));
    }


    bool valid(){
        return (AV_NOPTS_VALUE != timestamp_) && (rational_.num() != 0 && rational_.den() != 0);
    }
    operator bool(){
        return valid();
    }
    double seconds(){
        return rational_.as_double() * timestamp_;
    }

    double milliseconds(){
        return rational_.as_double() * timestamp_ * 1000;
    }

    FFTimestamp rescale(const FFRational& dest_rational = FFTimeBaseQ){
        int64_t t = av_rescale_q(timestamp_,rational_.value(),dest_rational.value());
        return FFTimestamp(t,dest_rational);
    }

    int64_t timestamp(){
        return timestamp_;
    }

    void set_timestamp(int64_t timestamp){
        timestamp_ = timestamp;
    }
private:
    int64_t timestamp_;
    FFRational rational_;
};


#endif // FFMPEG_H
