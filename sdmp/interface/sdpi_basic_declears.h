#ifndef SDP_FORWARD_DECLEAR_H
#define SDP_FORWARD_DECLEAR_H
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <atomic>
#include <inttypes.h>

#include "tinycom.h"

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




#define STDMETHODCALLTYPE

#define IMPLEMENT_FAKE_COM_OBJECT \
    virtual HRESULT QueryInterface(REFIID riid, void **ppvInterface){return S_OK;}\
    virtual ULONG  AddRef(){return S_OK;}\
    virtual ULONG  Release(){return S_OK;}

namespace sdp {
class IPin;
class IFilter;
class IGraph;
class Frame;

typedef tinycom::TGUID TGUID;
typedef tinycom::IUnknown IObject;
#define ComPointer tinycom::ComPtr
typedef ComPointer<IPin>    PinPointer;
typedef ComPointer<IFilter> FilterPointer;
typedef ComPointer<tinycom::IUnknown> IUnknownPointer;
typedef std::shared_ptr<Frame>   FramePointer;

typedef  std::vector<PinPointer> PinVector;

typedef void(*FrameReleaser)(AVFrame*,AVPacket*);
typedef FramePointer(*FrameSoftwareTransfer)(AVFrame*);

}

#include "sdpi_basic_types.h"
#endif // SDP_FORWARD_DECLEAR_H
