#ifndef IMGUI_HELPER_H
#define IMGUI_HELPER_H
#include <vector>
#include <string>
#include <map>
#include <imgui.h>

struct ImguiFontResource
{
    std::string ttf_file_path;
    const char* ttf_base85 = nullptr;
    void* ttf_binary_data = nullptr;
    uint32_t ttf_binary_size = 0;
    bool ttf_binary_compressed = false;
    float scale = 1.0;
    const ImWchar* range = nullptr;
    ImFontConfig config;
};
class ImGuiHelper{
public:
    static bool& pushed(){
        static bool s_pushed = false;
        return s_pushed;
    }
    static ImGuiHelper& get(const std::string& name = "defualt-font"){
        static std::map<std::string,ImGuiHelper> helpers;
        if(helpers.find(name) == helpers.end())
            helpers[name] = ImGuiHelper();
        return helpers[name];
    }
    void create_default_font(int32_t size,std::vector<ImguiFontResource> fonts){
        ImGuiIO& io = ImGui::GetIO();
        bool first = true;
        for(auto& item : fonts){
            item.config.MergeMode = !first;
            first = false;
            float font_size = size*item.scale;
            ImFont* font = nullptr;
            if(item.ttf_file_path.size()){
                font = io.Fonts->AddFontFromFileTTF(item.ttf_file_path.c_str(),font_size,&item.config,item.range);
            }
            else if(item.ttf_base85){
                font = io.Fonts->AddFontFromMemoryCompressedBase85TTF(item.ttf_base85,font_size,&item.config,item.range);
            }
            else if(item.ttf_binary_data){
                if(item.ttf_binary_compressed)
                    font = io.Fonts->AddFontFromMemoryCompressedTTF(item.ttf_binary_data,item.ttf_binary_size,font_size,&item.config,item.range);
                else
                    font = io.Fonts->AddFontFromMemoryTTF(item.ttf_binary_data,item.ttf_binary_size,font_size,&item.config,item.range);
            }
            if(!font_ && font){
                font_ = font;
                origin_size_ = font_size;
                if(font_->FontSize == 0){
                    //load will not set FontSize?
                    font_->FontSize = origin_size_;
                }
            }
        }
    }

    void build(){
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Build();
    }

    int basic_font_size(){
        if(font_)
            return font_->FontSize;
        return ImGui::GetFont()->FontSize;
    }
    void use_font_size(float size){
        if(!font_)
            return;
        float scale = size / origin_size_;
        use_font_scale(scale);
    }

    void use_font_scale(float scale){
        if(!font_)
            return;
        if(pushed_font_){
            ImGui::PopFont();
            pushed() = false;
        }
        font_->Scale = scale;
        ImGui::PushFont(font_);
        pushed() = true;
    }

    void restore_font_size(){
        if(font_)
            font_->Scale = 1;
        ImGui::PopFont();
        pushed() = false;
    }
private:
    ImFont* font_ = nullptr;
    bool pushed_font_ = false;
    int origin_size_ = 0;
};


#endif // IMGUI_HELPER_H
