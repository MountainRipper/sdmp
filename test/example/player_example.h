#ifndef PLAYEREXAMPLE_H
#define PLAYEREXAMPLE_H
#include <mutex>
#include <queue>
#include "example_base.h"
#include <easy/player.h>

using namespace mr;


struct CacheFrame{

    CacheFrame(const CacheFrame& other);
    CacheFrame(){

    }
    void create_to_texture();
    void release_texture();
public:
    uint32_t texture_y = 0;
    uint32_t texture_u = 0;
    uint32_t texture_v = 0;
    sdmp::FramePointer frame;
};


class PlayerExample : public ExampleBase
        ,public   sdmp::PlayerEvent
{
public:
    PlayerExample();

    virtual int32_t on_position(sdmp::Player *player, int64_t ms) override{
        return 0;
    }
    virtual int32_t on_video_frame(sdmp::Player *player, sdmp::FramePointer frame) override;
    CacheFrame pop_frame();
    int32_t cache_frame_count();
    int32_t totle_frame_count();
    // ExampleBase interface
public:
    virtual int32_t on_init(void *window) override;
    virtual int32_t on_deinit() override;
    virtual int32_t on_frame() override;
    virtual void button_callback( int bt, int action, int mods) override;
    virtual void cursor_callback( double x, double y) override;
    virtual void key_callback( int key, int scancode, int action, int mods) override;
    virtual void char_callback( unsigned int key) override;
    virtual void error_callback(int err, const char *desc) override;
    virtual void resize_callback( int width, int height) override;
    virtual void scroll_callback( double xoffset, double yoffset) override;
private:
    sdmp::Player* g_player;
    std::mutex cache_mutex_;
    std::queue<CacheFrame> cached_frames_;
    int32_t totle_frame_count_ = 0;

    uint32_t texture0_ = 0;
    uint32_t texture1_ = 0;
    uint32_t texture2_ = 0;
    uint32_t program_;
    uint32_t g_vao = 0;
};

#endif // PLAYEREXAMPLE_H
