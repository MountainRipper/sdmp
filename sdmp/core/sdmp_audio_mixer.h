#ifndef SDMP_AUDIO_MIXER_H
#define SDMP_AUDIO_MIXER_H


namespace mr::sdmp {

#include <vector>
#include <algorithm>
#include <type_traits>
#include <cstdint>
#include <limits>
#include <stdexcept>

#define USE_FFMPEG

#ifdef USE_FFMPEG
extern "C"{
    #include <libavutil/samplefmt.h>
}
#endif

class AudioMixer {
public:


#ifdef USE_FFMPEG
    enum Format{
        S16 = AV_SAMPLE_FMT_S16,
        S32 = AV_SAMPLE_FMT_S32,
        S64 = AV_SAMPLE_FMT_S64,
        F32 = AV_SAMPLE_FMT_FLT,
        F64 = AV_SAMPLE_FMT_DBL,
    };
#else
    //define same to ffmpeg
    enum PCMixerFormat{
        S16 = 1,
        S32 = 2,
        S64 = 10,
        F32 = 3,
        F64 = 4,
    };
#endif

    // 构造函数 (可选设置输出格式，默认为FLOAT)
    explicit AudioMixer(){

    }

    // 添加音频流到混音器
    template<typename T>
    void input_stream(const T* input, size_t samples, float volume = 1.0f) {
        if (volume < 0.0f || volume > 1.0f) {
            throw std::invalid_argument("Volume must be between 0.0 and 1.0");
        }

        // 确保混音缓冲区足够大
        if (mixed_.size() < samples) {
            mixed_.resize(samples);
        }

        // 根据输入类型进行混音
        for (size_t i = 0; i < samples; ++i) {
            mixed_[i] += to_double(input[i]) * volume;
        }
    }

    // 获取混音结果 (带自动裁剪)
    template<typename T>
    void get_output(T* output, size_t samples) {
        if (mixed_.size() < samples) {
            throw std::out_of_range("Requested more samples than available");
        }

        for (size_t i = 0; i < samples; ++i) {
            output[i] = scale_convert<T>(mixed_[i]);
        }

        // 清除已读取的样本
        mixed_.clear();
    }


    void input_stream(Format format, const uint8_t* input, size_t samples, float volume = 1.0f){
        if(format == S16){
            input_stream((const int16_t*)input, samples, volume);
        }
        else if(format == S32){
            input_stream((const int32_t*)input, samples, volume);
        }
        else if(format == S64){
            input_stream((const int64_t*)input, samples, volume);
        }
        else if(format == F32){
            input_stream((const float*)input, samples, volume);
        }
        else if(format == F64){
            input_stream((const double*)input, samples, volume);
        }
    }
    void get_output(Format format,uint8_t* output, size_t samples) {
        if(format == S16){
            get_output((int16_t*)output, samples);
        }
        else if(format == S32){
            get_output((int32_t*)output, samples);
        }
        else if(format == S64){
            get_output((int64_t*)output, samples);
        }
        else if(format == F32){
            get_output((float*)output, samples);
        }
        else if(format == F64){
            get_output((double*)output, samples);
        }
    }

private:
    std::vector<double> mixed_; // 内部使用double精度混音
    double scale_ = 1.0;

    // 将各种输入格式转换为double进行混音
    template<typename T>
    inline double to_double(T sample) {
        if constexpr (std::is_integral_v<T>) {
            // 整数类型归一化到[-1.0, 1.0]范围
            constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
            return static_cast<double>(sample) / (max + (std::numeric_limits<T>::is_signed ? 0.0 : 1.0));
        } else {
            // 浮点类型直接使用
            return static_cast<double>(sample);
        }
    }

    // 将混音结果裁剪并转换为目标格式
    template<typename T>
    inline T scale_convert(double sample) {

        //自适应混音加权,压缩
        sample *= scale_;
        if(abs(sample) > 1.0){
            scale_ = 1.0 / sample;
        }

        if constexpr (std::is_integral_v<T>) {
            // 整数类型处理
            constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
            constexpr double min = std::numeric_limits<T>::is_signed ?
                                       -max - 1.0 : 0.0;

            // 裁剪到[-1.0, 1.0]范围
            double clipped = std::clamp(sample, -1.0, 1.0);

            // 转换回整数范围
            return static_cast<T>(clipped * (max + (std::numeric_limits<T>::is_signed ? 0.0 : 1.0)));
        } else {
            // 浮点类型处理
            return static_cast<T>(std::clamp(sample, -1.0, 1.0));
        }

        //自适应混音加权,回归
        if(scale_ < 1.0){
            scale_ += (1.0 - scale_) / 32.0;
            scale_ = std::min(scale_, 1.0);
        }
    }
};


}
#endif // SDMP_AUDIO_MIXER_H
