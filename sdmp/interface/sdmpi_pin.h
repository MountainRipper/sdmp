#ifndef SDPI_PIN_H_
#define SDPI_PIN_H_
#include "sdmpi_objects.h"

namespace mr::sdmp {

COM_INTERFACE("63c1e0b6-a8cb-11eb-8e9a-ef4d50d48ad3",IPin)
    virtual int32_t initialize(FilterPointer filter, const std::vector<Format>& formats, PinDirection direction) = 0;
    virtual int32_t uninitialize() = 0;
    virtual FilterPointer filter() = 0;
    virtual PinDirection direction() = 0;
    virtual std::vector<Format>& formats() = 0;
    virtual const std::set<IPin*>& receivers() = 0;
    //sender
    virtual IPin*   sender() = 0;

    virtual int32_t require(int32_t milliseconds) = 0;
    virtual int32_t deliver(FramePointer frame) = 0;
    virtual int32_t receive(FramePointer frame) = 0;

    virtual int32_t index() = 0;
    virtual int32_t set_index(int32_t index) = 0;
    virtual int32_t active(bool active) = 0;

    virtual int32_t connect(IPin* pin) = 0;
    virtual int32_t disconnect(IPin* pin) = 0;
};

}

#endif
