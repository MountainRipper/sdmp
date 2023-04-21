#ifndef PLAYEREXAMPLE_H
#define PLAYEREXAMPLE_H
#include <tio/tio_types.h>
#include <mutex>
#include <queue>
#include <mr/sdl_runner.h>
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


class PlayerExample : public SDLShowcaseBase
        ,public   sdmp::PlayerEvent
{
public:
    PlayerExample();

    virtual int32_t on_video_frame(sdmp::Player *player, sdmp::FramePointer frame) override;
    virtual int32_t on_end() override;
    virtual int32_t on_reload() override;
    virtual int32_t on_playing() override;
    virtual int32_t on_replaying() override;
    virtual int32_t on_paused() override;
    virtual int32_t on_resumed() override;
    virtual int32_t on_stoped() override;

    CacheFrame pop_frame();
    int32_t cache_frame_count();
    int32_t totle_frame_count();
    // ExampleBase interface
public:
    virtual int32_t on_set_params(cxxopts::Options& options) override;
    virtual int32_t on_pre_init(cxxopts::ParseResult& options_result,uint32_t& window_flags) override;
    virtual int32_t on_init(void *window,int width, int height) override;
    virtual int32_t on_deinit() override;
    virtual int32_t on_frame() override;
    virtual void button_callback(int bt,int type,int clicks,double x,double y) override;
    virtual void cursor_callback(double x, double y) override;
    virtual void key_callback(int key, int type,int scancode,int mods) override;
    virtual void error_callback(int err, const char *desc) override;
    virtual void resize_callback(int width, int height) override;
    virtual void command(std::string command) override;
private:
    void render_ui();
private:
    sdmp::Player* g_player;
    std::mutex cache_mutex_;
    std::queue<CacheFrame> cached_frames_;
    int32_t totle_frame_count_ = 0;

    uint32_t textures_[4] = {0};
    uint32_t texture_locations_[4] = {0};
    uint32_t texture_unit_base_ = 2;
    std::shared_ptr<mr::tio::ReferenceShader> shader_;
    uint32_t g_vao = 0;


    //ui stuff
    int width_ = 0;
    int height_ = 0;
    int  channel_mode_ = 0;
    int  select_track_ = 0;
    bool resized_ = true;

    int32_t duration_ = 0;
    int8_t tracks_ = 0;
    float position_ = 0;
    float seek_position_ = -1;
    bool auto_replay_ = false;
    std::string status_string_ = "Idel";
    std::vector<std::string> status_string_history_;
};

#endif // PLAYEREXAMPLE_H
