#include <algorithm>
#include "sdp_timeline_support.h"
using namespace mr::sdmp;


GeneralTimeline::GeneralTimeline()
{

}

int64_t GeneralTimeline::set_timeline(int64_t time)
{
    //auto set running status when set time
    timeline_status_ = kTimelineRunning;
    timeline_ = time;
    return timeline_;
}

int64_t GeneralTimeline::get_timeline()
{
    return timeline_;
}

IFilterExtentionTimeline::TimelineStatus GeneralTimeline::get_timeline_status()
{
    return timeline_status_;
}

int32_t GeneralTimeline::set_timeline_status(TimelineStatus status)
{
    timeline_status_ = status;
    return 0;
}

int32_t TimelineDecisionGroup::append_timeline(IFilterExtentionTimeline *timeline)
{
    start_time_point_ = MR_TIMER_NOW;
    timelines_.insert(timeline);
    return 0;
}

int32_t TimelineDecisionGroup::remove_timeline(IFilterExtentionTimeline *timeline)
{
    timelines_.erase(timeline);
    return 0;
}

int32_t TimelineDecisionGroup::clear_timeline()
{
    timelines_.clear();
    last_calculation_timeline_ = 0;
    last_calculation_tick_ = 0;
    return 0;
}

int64_t TimelineDecisionGroup::calculation_current_timeline()
{
    int64_t max_val = 0;
    for(auto item : timelines_){
        if(item->get_timeline_status() != IFilterExtentionTimeline::kTimelineDisactive){
            auto val = item->get_timeline();
            max_val = std::max(max_val,val);
        }
    }
    last_calculation_timeline_ = max_val;
    last_calculation_tick_ = MR_TIMER_MS(start_time_point_);
    return max_val;
}

int64_t TimelineDecisionGroup::timeline_current()
{
    return last_calculation_timeline_;
}

int64_t TimelineDecisionGroup::timeline_current_in_linear()
{
    return last_calculation_timeline_ + (MR_TIMER_MS(start_time_point_) - last_calculation_tick_);
}

IFilterExtentionTimeline::TimelineStatus TimelineDecisionGroup::status()
{
    IFilterExtentionTimeline::TimelineStatus status = IFilterExtentionTimeline::kTimelineNone;
    for(auto item : timelines_){
        if(item->get_timeline_status() == IFilterExtentionTimeline::kTimelineBlocking)
            return IFilterExtentionTimeline::kTimelineBlocking;


    }
    return status;
}
