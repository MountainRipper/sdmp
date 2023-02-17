#ifndef SDPI_BASIC_TYPES_H
#define SDPI_BASIC_TYPES_H
#include <any>
#include <string>
#include <memory>
#include <map>
#include <string.h>
#include <vector>

#ifdef _WIN64
   #define SDP_OS "windows"
#elif _WIN32
   #define SDP_OS "windows"
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
        #define SDP_OS "ios"
    #elif TARGET_OS_IPHONE
        #define SDP_OS "ios"
    #else
        #define TARGET_OS_OSX 1
        #define SDP_OS "macos"
    #endif
#elif __ANDROID__
    #define SDP_OS "android"
#elif __linux__
    #define SDP_OS "linux"
#elif __unix__
    #define SDP_OS "unix"
#elif __posix
    #define SDP_OS "posix"
#endif

//https://sourceforge.net/p/predef/wiki/Architectures/
#if (__arm__ || _M_ARM)
    #define SDP_ARCH "arm"
#elif (__aarch64__)
    #define SDP_ARCH "aarch64"
#elif (__x86_64__ || __amd64__ || _M_AMD64 || _M_X64)
    #define SDP_ARCH "x86_64"
#elif (__X86__ || _X86_ || __i386__ || __i386 || _M_IX86)
    #define SDP_ARCH "x86"
#elif (__MIPS__ || __mips__ || __mips )
    #define SDP_ARCH "mips"
#endif


extern "C"{
struct AVPacket;
struct AVFrame;
}
namespace sol {
class state;
struct lua_value;
}

namespace mr::sdmp {


#define kSuccessed                          0
#define kErrorInvalidParameter              -1
/*
Error Codes
*/
#define kErrorBadScript                     -10000
#define kErrorCreateGraphScriptObject       -10001
#define kErrorCreateGraphFilterObject       -10002
#define kErrorConnectFailedNoOutputPin      -10003
#define kErrorConnectFailedNoInputPin       -10004
#define kErrorConnectFailedNotMatchFormat   -10005
#define kErrorConnectFailedInputPinAlreadyConnected       -10006
#define kErrorConnectFailedNotMatchInOut    -10007

#define kErrorFilterInvalid                 -20000
#define kErrorFilterEos                     -20001
#define kErrorFilterDecode                  -20002
#define kErrorFilterNoInterfce              -20003

#define kErrorFormatNotAcceped              -30000
#define kErrorStatusNotAcceped              -30001

#define kErrorResourceNotFound              -40000
#define kErrorResourceBusy                  -40002
#define kErrorResourceOpen                  -40003
#define kErrorResourceRead                  -40004

/*
 * keys for C++ passed to Lua objects
*/
#define kGraphKey                "graph" //lua object from createGraph passed back to as global 'graph'

/*
 * keys for Lua passed to C++ objects
*/
#define kLuaCreateGraphFunctionName         "createGraph"
#define kLuaCreatedEventFunctionName        "onCreated"
#define kLuaMasterLoopEventFunctionName     "onMasterClock"
#define kLuaErrorEventFunctionName          "onError"
#define kLuaStatusEventFunctionName         "onStatus"
#define kLuaConnectEventFunctionName        "onConnect"
#define kLuaConnectDoneEventFunctionName    "onConnectDone"
#define kLuaPositionEventFunctionName       "onPosition"

/*
 * internal properties for Graph, with prop-* for read only
*/
#define kGraphPropertyCurrentPts        "prop-pts"

/*
 * commands for Lua call to C++ graph
*/
#define kGraphCommandConnect            "cmdConnect"
#define kGraphCommandDisconnect         "cmdDisconnect"
//control commands also pass to filter
#define kGraphCommandPlay               "cmdPlay"
#define kGraphCommandPause              "cmdPause"
#define kGraphCommandStop               "cmdStop"
#define kGraphCommandSeek               "cmdSeek"
#define kGraphCommandClose              "cmdClose"

#define kGraphOperatorConnect           "doConnect"
#define kGraphOperatorCreateFilter      "doCreateFilter"
#define kGraphOperatorRemoveFilter      "doRemoveFilter"
#define kGraphOperatorSetFilterProperty "setFilterProperty"
#define kGraphOperatorCallFilterMethod  "callFilterMethod"

#define kGraphInvalidPts         INT32_MIN
enum GraphStatus {
    kStatusNone  = 0 ,
    kStatusInit   ,
    kStatusReady  ,
    kStatusRunning,
    kStatusPaused ,
    kStatusStoped ,
    kStatusEos
};
typedef GraphStatus GraphStatus;

static inline GraphStatus command_cause_status(const std::string& command){

    if(command == kGraphCommandConnect) return kStatusReady;
    else if(command == kGraphCommandDisconnect) return kStatusInit;
    else if(command == kGraphCommandPlay) return kStatusRunning;
    else if(command == kGraphCommandPause) return kStatusPaused;
    else if(command == kGraphCommandStop) return kStatusStoped;
    else if(command == kGraphCommandSeek) return kStatusReady;
    else if(command == kGraphCommandClose) return kStatusStoped;
    return kStatusNone;
}

/*
 * commands for Graph call to Filters
*/
//graph changed status, param is (kGraphCommandPlay/Pause/Stop)
#define kFilterCommandStatus            "filterCommandStatus"
//master loop request data,from link's tail(output) to head(input) , return (kFilterResultRequestSenderStream) if needed
#define kFilterCommandRequestStream     "filterCommandRequestStream"
//master loop check end of stream, return (kFilterResultRequestSenderStream) if needed
#define kFilterCommandCheckEndOfStream  "filterCommandCheckEndOfStream"
//graph seek to new position, runing a new segment, param is timeline position
#define kFilterCommandNewSegment        "filterCommandNewSegment"
//graph connecting active the filter
#define kFilterCommandActive            "filterCommandActive"
//graph connecting inactive the filter
#define kFilterCommandInactive          "filterCommandInactive"

/*
 * return code for filters
*/
#define kFilterResultEndOfStream          -100
#define kFilterResultRequestSenderStream  -101
/*
 * internal properties for Filter, with prop-* for read only
*/
//get device name
#define kFilterPropertyDevice       "prop-device"
#define kFilterPropertyCurrentPts   "prop-pts"
#define kFilterPropertyStatus       "prop-status"

/*
 * value defines for filters
*/
#define kFilterLevelInvalid         INT32_MAX

enum FilterType{
    kFilterTypeNone           = 0,
    kFilterTypeAudioInput     = 1,
    kFilterTypeVideoInput     = 1<<1,
    kFilterTypeAudioDecoder   = 1<<2,
    kFilterTypeVideoDecoder   = 1<<3,
    kFilterTypeAudioEncoder   = 1<<4,
    kFilterTypeVideoEncoder   = 1<<5,
    kFilterTypeAudioProcessor = 1<<6,
    kFilterTypeVideoProcessor = 1<<7,
    kFilterTypeAudioOutput    = 1<<8,
    kFilterTypeVideoOutput    = 1<<9
};

static inline FilterType filter_type_from_string(const char* type_string){
    if(strcmp(type_string,"AudioInput")==0) return kFilterTypeAudioInput;
    else if(strcmp(type_string,"VideoInput")==0) return kFilterTypeVideoInput;
    else if(strcmp(type_string,"AudioDecoder")==0) return kFilterTypeAudioDecoder;
    else if(strcmp(type_string,"VideoDecoder")==0) return kFilterTypeVideoDecoder;
    else if(strcmp(type_string,"AudioEncoder")==0) return kFilterTypeAudioEncoder;
    else if(strcmp(type_string,"VideoEncoder")==0) return kFilterTypeVideoEncoder;
    else if(strcmp(type_string,"AudioProcessor")==0) return kFilterTypeAudioProcessor;
    else if(strcmp(type_string,"VideoProcessor")==0) return kFilterTypeVideoProcessor;
    else if(strcmp(type_string,"AudioOutput")==0) return kFilterTypeAudioOutput;
    else if(strcmp(type_string,"VideoOutput")==0) return kFilterTypeVideoOutput;
    return kFilterTypeNone;
}

#define FILTER_IS_INPUT(filter) ((filter&kFilterTypeAudioInput)||(filter&kFilterTypeVideoInput))
#define FILTER_IS_DECODER(filter) ((filter&kFilterTypeAudioDecoder)||(filter&kFilterTypeVideoDecoder))
#define FILTER_IS_ENCODER(filter) ((filter&kFilterTypeAudioEncoder)||(filter&kFilterTypeVideoEncoder))
#define FILTER_IS_PROCESS(filter) ((filter&kFilterTypeAudioProcessor)||(filter&kFilterTypeVideoProcessor))
#define FILTER_IS_OUTPUT(filter) ((filter&kFilterTypeAudioOutput)||(filter&kFilterTypeVideoOutput))


class IPin;
class IFilter;
class IGraph;
class Frame;
typedef std::shared_ptr<Frame>   FramePointer;
typedef void(*FrameReleaser)(AVFrame*,AVPacket*);
typedef FramePointer(*FrameSoftwareTransfer)(AVFrame*);
typedef std::map<std::string,std::string> FeatureMap;
struct Rational{
    int numerator = 0;
    int denominator = 0;
};

enum PinDirection{
    kInputPin,
    kOutputPin
};
struct PinIndex{
    IPin *pin = nullptr;
    PinDirection direction = kInputPin;
    int32_t index = -1;
};
#define kPinIndexAll  INT32_MIN


struct Format{
    int32_t type          = -1;     //as AVMediaType
    int32_t codec         = -1;     //as AVCodecID
    int32_t format        = -1;     //as AVPixelFormat/AVSampleFormat
    int32_t bitrate       = 0;

    int32_t width         = 0;
    int32_t height        = 0;
    float   fps           = 0;

    int32_t channels      = 0;
    int32_t samplerate    = 0;

    Rational timebase;

    void*   codec_parameters = nullptr;//as AVCodecParameters
    void*   codec_context    = nullptr;//as AVCodecContext(almost not need, keep for test)

    bool isVideo()const {return type == 0;}
    bool isAudio() const {return type == 1;}
    bool isSubtitle()const {return type == 3;}
};

enum FrameFlags{
    kFrameFlagNone= 0,
    kFrameFlagEos = 1,
    kFrameFlagPreview = 2,
};
enum RotationMode {
  kRotate0 = 0,      // No rotation.
  kRotate90 = 90,    // Rotate 90 degrees clockwise.
  kRotate180 = 180,  // Rotate 180 degrees.
  kRotate270 = 270,  // Rotate 270 degrees clockwise.
};

class Frame{
public:

    ~Frame(){
        if(releaser){
            releaser(frame,packet);
        }
    }
    static FramePointer make_frame(AVFrame* frame){
        FramePointer new_frame= FramePointer(new Frame());
        new_frame->frame = frame;
        return new_frame;
    }
    static FramePointer make_packet(AVPacket* packet){
        FramePointer new_frame= FramePointer(new Frame());
        new_frame->packet = packet;
        return new_frame;
    }

    FramePointer transfer_software_frame(){
        if(transfer == nullptr || frame == nullptr)
            return FramePointer();
        return transfer(frame);
    }
    FrameReleaser releaser           = nullptr;
    FrameSoftwareTransfer transfer   = nullptr;
    AVPacket* packet                 = nullptr;
    AVFrame*  frame                  = nullptr;
    FrameFlags  flag                 = kFrameFlagNone;
    RotationMode rotation            = kRotate0;
};


enum ValueType: uint8_t{
    kPorpertyNone        = 0,
    kPorpertyNumber      = 1,
    kPorpertyString      = 2,
    kPorpertyBool        = 3,
    kPorpertyNumberArray = 4,
    kPorpertyStringArray = 5,
    kPorpertyPointer     = 6,
    kPorpertyLuaFunction = 7,
    kPorpertyLuaTable    = 8
};

#define ANY2F64(V)      std::any_cast<double>(V)
#define ANY2STR(V)      std::any_cast<const std::string&>(V)
#define ANY2BOOL(V)     std::any_cast<bool>(V)
#define ANY2F64ARR(V)   std::any_cast<const std::vector<double>&>(V)
#define ANY2STRARR(V)   std::any_cast<const std::vector<std::string>&>(V)
#define ANY2PTR(V)      std::any_cast<void*>(V)
#define ANY2LUAFUN(V)   std::any_cast<sol::function>(V)
#define ANY2LUATABLE(V) std::any_cast<sol::table>(V)

#define VALUE_OPERATOR_TYPE(T)\
operator T () const{\
    return std::any_cast<T>(value_);\
}

#define VALUE_CONSTRUCTOR(T,ASNAME)\
Value(const T& v,const std::string& name = "",bool readonly = false)\
    :name_(name)\
    ,readonly_(readonly){\
    value_ = v;\
    type_ = type_of_typeid(value_.type());\
}\
operator T () const{\
    return std::any_cast<T>(value_);\
}\
const T as_##ASNAME () const{\
    return std::any_cast<T>(value_);\
}

#define VALUE_CONSTRUCTOR_TYPE(FROM,TO,ASNAME) \
Value(const FROM& v,const std::string& name = "",bool readonly = false)\
    :name_(name)\
    ,readonly_(readonly){\
    value_ = static_cast<TO>(v);\
    type_ = type_of_typeid(value_.type());\
}\
operator FROM () const{\
    return static_cast<FROM>(std::any_cast<TO>(value_));\
}\
const FROM as_##ASNAME () const{\
    return static_cast<FROM>(std::any_cast<TO>(value_));\
}

class Value{
public:
    Value(){

    }
    template<typename T>Value(const T& v,const std::string& name = "",bool readonly = false)
        :name_(name)
        ,readonly_(readonly){
        value_ = v;
        type_ = type_of_typeid(value_.type());
    }

    VALUE_CONSTRUCTOR_TYPE(float   ,double,float)
    VALUE_CONSTRUCTOR_TYPE(int8_t  ,double,int8)
    VALUE_CONSTRUCTOR_TYPE(uint8_t ,double,uint8)
    VALUE_CONSTRUCTOR_TYPE(int16_t ,double,int16)
    VALUE_CONSTRUCTOR_TYPE(uint16_t,double,uint16)
    VALUE_CONSTRUCTOR_TYPE(int32_t ,double,int32)
    VALUE_CONSTRUCTOR_TYPE(uint32_t,double,uint32)
    VALUE_CONSTRUCTOR_TYPE(int64_t ,double,int64)
    VALUE_CONSTRUCTOR_TYPE(uint64_t,double,uint64)

    VALUE_CONSTRUCTOR(double,double)
    VALUE_CONSTRUCTOR(bool,bool)
    VALUE_CONSTRUCTOR(void*,pointer)
    VALUE_CONSTRUCTOR(std::string,string)
    VALUE_CONSTRUCTOR(std::vector<double>,double_vector)
    VALUE_CONSTRUCTOR(std::vector<std::string>,string_vector)

    Value(const Value& other){
        type_ = other.type_;
        name_ = other.name_;
        value_ = other.value_;
        readonly_ = other.readonly_;
    }
    Value& operator=(const Value& other){
        type_ = other.type_;
        name_ = other.name_;
        value_ = other.value_;
        readonly_ = other.readonly_;
        value_changed_ = true;
        return *this;
    }

    template<class T>
    operator T() const{
        return std::any_cast<T>(value_);
    }

    template<class T>
    T as() const{
        return std::any_cast<T>(value_);
    }

    bool valid_sdmp_value() const {
        return type_ != kPorpertyNone;
    }

    int32_t to_lua_value(sol::state* state,sol::lua_value* lua_value_ptr){
        return any_to_lua(value_,state,lua_value_ptr);
    }
    int32_t from_lua_value(const sol::lua_value* lua_value_ptr){
        lua_to_any(lua_value_ptr,value_);
        type_ = type_of_typeid(value_.type());
        return 0;
    }
    static Value create_from_lua_value(const sol::lua_value* lua_value_ptr){
        Value value;
        value.from_lua_value(lua_value_ptr);
        return value;
    }
    static const std::type_info& typeid_of_type(ValueType type);
    static ValueType type_of_typeid(const std::type_info &typeinfo);
    static int32_t lua_to_any(const sol::lua_value *lua_value_ptr, std::any& any_value);
    static int32_t any_to_lua(const std::any& any_value,sol::state* state,sol::lua_value* lua_value_ptr);
public:
    ValueType type_ =kPorpertyNone;
    bool readonly_ = false;
    bool value_changed_ = false;
    std::string name_;
    std::any value_;
};

class ISdpStub{
public:
    virtual const std::string& module_name() = 0;
    virtual int32_t destory() = 0;
};


}


#endif // SDPI_BASIC_TYPES_H
