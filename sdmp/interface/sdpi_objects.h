#ifndef SDP_FORWARD_DECLEAR_H
#define SDP_FORWARD_DECLEAR_H
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <atomic>
#include "tinycom.h"
#include "sdpi_basic_types.h"

namespace mr::sdmp {

#define ComPointer tinycom::ComPtr
typedef tinycom::TGUID TGUID;
typedef tinycom::IUnknown IObject;
typedef ComPointer<IPin>    PinPointer;
typedef ComPointer<IFilter> FilterPointer;
typedef ComPointer<tinycom::IUnknown> IUnknownPointer;
typedef std::vector<PinPointer> PinVector;

}

#endif // SDP_FORWARD_DECLEAR_H
