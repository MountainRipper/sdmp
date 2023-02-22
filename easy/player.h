#ifndef PLAYER_H
#define PLAYER_H
#include <cstdint>
#include <string>
#include "sdpi_basic_types.h"
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
class Player
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

    int32_t set_video_emit_mode(bool pull_push);
private:    
    PlayerPrivateContex* context_ = nullptr;
};


}//namespace mr::sdmp
}//namespace mountain-ripper

#endif // PLAYER_H
