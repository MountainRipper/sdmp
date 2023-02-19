#ifndef PLAYER_H
#define PLAYER_H
#include <cstdint>
#include <string>
#include "sdpi_basic_types.h"
#include "sdpi_graph.h"
namespace mr {
namespace sdmp {

enum SdpPlayerCannelMode{
    SdpBothChannel = 0,
    SdpLeftChannel,
    SdpRightChannel
};

class Player;
class PlayerEvent{
public:
    virtual int32_t on_position(Player* player, int64_t ms) = 0;
    virtual int32_t on_video_frame(Player* player,sdmp::FramePointer frame) = 0;
};

class PlayerPrivateContex;
class Player : public IGraphEvent
{
public:
    Player(const std::string& base_scipts_dir, const std::string& easy_scipts_dir);
    int32_t set_event(PlayerEvent* event);

    int32_t open(const std::string& uri);
    int32_t play();
    int32_t pause();
    int32_t resume();
    int32_t stop();
    int32_t set_replay(bool replay);

    int32_t position();
    int32_t set_position(int32_t ms);
    float volume();
    int32_t set_volume(float volume);

    int32_t duration();
    float   framerate();
    int32_t width();
    int32_t height();
    float   samplerate();
    int32_t channels();

    int32_t tracks_count();
    int32_t track_samplerate(int32_t index);
    int32_t track_channels(int32_t index);
    int32_t set_track(int32_t index);
    // IGraphEvent interface
public:
    virtual int32_t on_graph_init(IGraph *graph) override;
    virtual int32_t on_graph_created(IGraph *graph) override;
    virtual int32_t on_graph_error(IGraph *graph, int32_t error_code) override;
    virtual int32_t on_graph_master_loop(IGraph *graph) override;
private:
    int32_t lua_call(const std::string& name,Value& value);
private:
    PlayerPrivateContex* context_;
};


}//namespace mr::sdmp
}//namespace mountain-ripper

#endif // PLAYER_H
