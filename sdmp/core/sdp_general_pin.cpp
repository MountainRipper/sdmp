#include "sdp_general_pin.h"
#include <sdpi_filter.h>
#include <logger.h>

using namespace sdp;

COM_REGISTER_OBJECT(GeneralPinObject)

GeneralPin::GeneralPin()
{

}

GeneralPin::~GeneralPin()
{

}

int32_t GeneralPin::initialize(FilterPointer filter, const std::vector<Format> &formats, PinDirection direction)
{
    filter_ = filter;
    direction_ = direction;
    formats_ = formats;
    return 0;
}
int32_t GeneralPin::uninitialize()
{
    filter_ = nullptr;
    if (sender_){
        sender_->disconnect(this);
    }

    for(auto pin : receivers_)
        pin->disconnect(this);

    sender_ = nullptr;
    receivers_.clear();
    return 0;
}


PinDirection GeneralPin::direction(){
    return direction_;
}

std::vector<Format> &GeneralPin::formats(){
    return formats_;
}

const std::set<IPin *> &GeneralPin::receivers()
{
    return receivers_;
}

IPin *GeneralPin::sender()
{
    return sender_;
}

int32_t GeneralPin::require(int32_t milliseconds)
{
    PinIndex index;
    index.direction = direction_;
    index.index = index_;
    index.pin = this;
    std::vector<PinIndex> indexs;
    indexs.push_back(index);
    return  filter_->requare(milliseconds,indexs);
}

FilterPointer GeneralPin::filter(){
    return filter_;
}

int32_t GeneralPin::connect(IPin *pin){

    if(pin == nullptr)
        return kErrorInvalidParameter;

    if(direction_ == kInputPin){
        if(pin->direction() != kOutputPin)
            return kErrorConnectFailedNotMatchInOut;
        if(sender_ != nullptr)
            return kErrorConnectFailedInputPinAlreadyConnected;
        sender_ = pin;
    }
    else{
        if(pin->direction() != kInputPin)
            return kErrorConnectFailedNotMatchInOut;
        receivers_.insert(pin);
    }
    return 0;
}

int32_t GeneralPin::disconnect(IPin *pin){
    MP_LOG_DEAULT("pin[{}-{}] disconnect [{}-{}]",direction_==kInputPin?"in":"out",(void*)this,
                   direction_==kInputPin?"out":"int",(void*)pin);
    if(direction_ == kInputPin){
        if(pin == sender_){
            sender_ = nullptr;
            return 0;
        }
        else
            return kErrorInvalidParameter;
    }
    else{
        if(pin)
            receivers_.erase(pin);
        else
            receivers_.clear();
    }
    return 0;
}

int32_t GeneralPin::index()
{
    return index_;
}

int32_t GeneralPin::set_index(int32_t index)
{
    index_ = index;
    return 0;
}

int32_t GeneralPin::active(bool active)
{
    active_ = active;
    return 0;
}

int32_t GeneralPin::deliver(FramePointer frame){
    if(direction_ != kOutputPin)
        return -1;

    //pass eos frame when inactive
    if((!active_ || !filter_->activable()) && !(frame->flag & kFrameFlagEos))
        return 0;

    if(!filter_->activable())
        return 0;

    for(auto& item : receivers_){
        item->receive(frame);
    }
    return 0;
}
int32_t GeneralPin::receive(FramePointer frame){
    if(direction_ != kInputPin)
        return -1;

    //pass eos frame when inactive
    if((!active_ || !filter_->activable()) && !(frame->flag & kFrameFlagEos))
        return 0;

    filter_->receive(this,frame);
    return 0;
}
