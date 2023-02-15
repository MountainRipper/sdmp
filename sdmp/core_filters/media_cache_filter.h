#ifndef MEDIACACHESAVEFILTER_H
#define MEDIACACHESAVEFILTER_H

#include "sdp_general_filter.h"
namespace mr::sdmp {

class MediaCacheManager;
COM_MULTITHREADED_OBJECT(
"13ddc120-a20f-11ed-a4be-cfa04ca70985",
R"({
  "clsid": "13ddc120-a20f-11ed-a4be-cfa04ca70985",
  "describe": "media cache/save from IFilterExtentionMediaCacheSource linked",
  "filtertype": ["AudioProcessor","VideoProcessor"],
  "name": "mediaCacheSaver",
  "properties": [
    {
      "name": "cacheDir",
      "type": "string",
      "value": ""
    }
  ],
  "type": "sdp-filter"
})",
MediaCacheSaverFilter)
, public GeneralFilterObjectRoot<MediaCacheSaverFilter>
{
public:
    MediaCacheSaverFilter();

    COM_MAP_BEGINE(MediaCacheSaverFilter)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const table &config) override;
    virtual int32_t connect_before_match(IFilter *sender_filter);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin) override;
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index) override;
    virtual int32_t receive(IPin *input_pin, FramePointer frame) override;
    virtual int32_t requare(int32_t duration, const std::vector<PinIndex> &output_pins) override;
    virtual int32_t process_command(const std::string &command, const Value &param) override;

    virtual int32_t property_changed(const std::string &property, const Value &value);
    virtual Value call_method(const std::string &method, const Value &param);
private:
    std::string get_cache_file_of_uri(std::string uri);
private:
    IFilter* sender_filter_ = nullptr;
    ComPointer<IFilterExtentionMediaCacheSource> cache_source_ = nullptr;
    IFilterExtentionMediaCacheSource::Info source_info_;

    std::shared_ptr<MediaCacheManager> manager_;
    bool pass_though_mode_ = false;
    std::map<IPin*,std::pair<IPin*,IPin*>> source_pin_map_;
};

}

#endif // MEDIACACHESAVEFILTER_H
