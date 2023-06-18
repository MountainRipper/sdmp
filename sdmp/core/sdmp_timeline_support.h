#ifndef TIMELINESUPPORT_H
#define TIMELINESUPPORT_H
#include <inttypes.h>
#include <set>
#include <chrono>
#include <mrcommon/logger.h>
#include "sdmpi_filter_extentions.h"

namespace mr::sdmp {

class GeneralTimeline : public IFilterExtentionTimeline
{
public:
    GeneralTimeline();
    int64_t set_timeline(int64_t time);
    int64_t get_timeline();

    TimelineStatus get_timeline_status();
    int32_t set_timeline_status(TimelineStatus status);
private:
    int64_t         timeline_   = 0;
    TimelineStatus  timeline_status_ = kTimelineNone;
};


class TimelineDecisionGroup{
public:
    int32_t append_timeline(IFilterExtentionTimeline* timeline);
    int32_t remove_timeline(IFilterExtentionTimeline* timeline);
    int32_t clear_timeline();

    int64_t calculation_current_timeline();
    int64_t timeline_current();
    int64_t timeline_current_in_linear();

    IFilterExtentionTimeline::TimelineStatus status();
private:
    std::set<IFilterExtentionTimeline*> timelines_;
    int64_t last_calculation_timeline_  = 0;
    int64_t last_calculation_tick_      = 0;

    MR_TIMER_DEFINE(start_time_point_);
};

}


#endif // TIMELINESUPPORT_H
