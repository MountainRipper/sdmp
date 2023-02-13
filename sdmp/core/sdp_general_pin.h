#ifndef GENERALPIN_H
#define GENERALPIN_H
#include <sdpi_pin.h>

namespace sdp {

class GeneralPin : public IPin{
    friend class FilterBase;
public:
    GeneralPin();
    virtual ~GeneralPin();

    virtual int32_t initialize(FilterPointer filter, const std::vector<Format>& formats, PinDirection direction);
    virtual int32_t uninitialize();
    virtual FilterPointer filter();
    virtual PinDirection direction();
    virtual std::vector<Format>& formats();
    virtual const std::set<IPin*>& receivers();
    //sender
    virtual IPin* sender();

    virtual int32_t require(int32_t milliseconds);
    virtual int32_t deliver(FramePointer frame);
    virtual int32_t receive(FramePointer frame);

    virtual int32_t index();
    virtual int32_t set_index(int32_t index);
    virtual int32_t active(bool active);
    //connect/disconnect MUST use by filter,DO NOT call it manually
    virtual int32_t connect(IPin* pin);
    virtual int32_t disconnect(IPin* pin);
protected:
    IFilter*          filter_;
    IPin*                  sender_          = nullptr;
    std::set<IPin*>        receivers_;
    PinDirection           direction_       = kInputPin;
    std::vector<Format>    formats_;
    int32_t                index_           = -1;
    bool                   active_          = true;
};

COM_MULTITHREADED_OBJECT("d7326362-a212-11ed-8707-c78229224908","",GeneralPinObject)
,public GeneralPin{
    COM_MAP_BEGINE(GeneralPinObject)
            COM_INTERFACE_ENTRY(IPin)
    COM_MAP_END()
};

}

#endif // GENERALPIN_H
