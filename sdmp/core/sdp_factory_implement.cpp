#include <filesystem>
#include "sdp_factory_implement.h"
#include "sdp_graph_implement.h"
#include "sdp_objects.h"

#include "avcpp/av.h"
#include "sdpi_factory.h"

#include <nlohmann/json.hpp>
#include <logger.h>

namespace sdp {

std::shared_ptr<FactoryImplement> sdp::Factory::factory_;

#define DUMP_META(CLS) \
    extern int32_t g_register_filter_##CLS();\
    g_register_filter_##CLS();

COM_MODULE_BEGINE("5b8aab92-a452-11ed-bcb9-9b38750c5c76",SDP_INTERNAL_COM_MODULE)
    COM_MODULE_OBJECT_ENTRY(AudioOutputDeviceMiniaudioFilter)

    //common filter
    COM_MODULE_OBJECT_ENTRY(MediaSourceFFmpegFilter)
    COM_MODULE_OBJECT_ENTRY(MediaSourceMemoryFilter)
    COM_MODULE_OBJECT_ENTRY(VideoCaptureFFmpegFilter)

    COM_MODULE_OBJECT_ENTRY(MediaCacheSaverFilter)
    COM_MODULE_OBJECT_ENTRY(AudioEncoderFFmpegFilter)
    COM_MODULE_OBJECT_ENTRY(AudioDecoderFFmpegFilter)
    COM_MODULE_OBJECT_ENTRY(VideoEncoderFFmpegFilter)
    COM_MODULE_OBJECT_ENTRY(VideoDecoderFFmpegFilter)
    COM_MODULE_OBJECT_ENTRY(DataGrabber)
    #if defined(HAS_ROCKCHIP_MPP)
    COM_MODULE_OBJECT_ENTRY(VideoDecoderRkmppFilter)
    #endif

    COM_MODULE_OBJECT_ENTRY(AudioOutputParticipantFilter)
    COM_MODULE_OBJECT_ENTRY(VideoOutputProxyFilter)
    COM_MODULE_OBJECT_ENTRY(MediaMuxerFFmpegFilter)
COM_MODULE_END()

int32_t sdp::Factory::initialize_factory()
{
    mp::Logger::set_level(SDMP_LOG_LEVEL);
    //spdlog::set_level(spdlog::level::trace);
    MP_LOG_DEAULT("Factory::initialize_factory");
    factory_.reset(new FactoryImplement());
    return 0;
}

int32_t sdp::Factory::initialnize_engine(const std::string &root_dir,
                                         const std::string& declear_file,
                                         const FeatureMap& features)
{
    av::init();
    av_log_set_callback([](void*, int level, const char* format, va_list valist)
    {
        if(level > AV_LOG_INFO)
            return;
        char msg[1024];
        char format_temp[1024];
        strcpy(format_temp,format);
        if(format_temp[strlen(format_temp)-1] == '\n')
            format_temp[strlen(format_temp)-1]='\0';
        vsprintf(msg,format_temp,valist);
        MP_LOG_DEAULT("ffmpeg: {}",msg);
    });

    // factory can be created seperatly befor initialize_engine.
    if(!factory_)
        factory_.reset(new FactoryImplement());
    return  factory_->initialnize_engine(root_dir,declear_file,features);
}

const std::string &Factory::script_root()
{
    static const std::string empty_string;
    if(!factory_)
        return empty_string;
    return factory_->script_dir();
}


std::shared_ptr<IGraph> Factory::create_graph_from(const std::string &declear_file, IGraphEvent *event)
{
    auto graph = new GraphImplement(event);
    graph->create(factory_->script_dir(),declear_file);
    return std::shared_ptr<IGraph>(graph);
}

ISdpStub *Factory::create_stub(const std::string &class_name)
{
    if(class_name == "VideoRenderStubOpenGL"){
        //return static_cast<ISdpStub*>(new VideoRenderStubOpenGL());
    }
# if defined(__linux__)
    if(class_name == "VideoRenderStubKms"){
        return static_cast<ISdpStub*>(new VideoRenderStubKms());
    }
#endif
    return nullptr;
}

IUnknownPointer Factory::create_object(const TGUID &clsid)
{
    IUnknownPointer object;
    if(!factory_)
    {
        MP_ERROR("Factory::register_new_filter failed because of factory_ is not created");
        return object;
    }
    object = factory_->create_object(clsid);
    return object;
}

std::shared_ptr<FactoryImplement> Factory::factory()
{
    return factory_;
}

const char* Factory::object_metadata(const TGUID &clsid) {
    return factory_->object_metadata(clsid);
}

const FilterDelear *Factory::filter_declear(const TGUID &clsid)
{
    return factory_->filter_declear(clsid);
}


FactoryImplement::FactoryImplement(){
    COM_MODULE_IMPORT(SDP_INTERNAL_COM_MODULE);
    auto internal_module = COM_MODULE_GET(SDP_INTERNAL_COM_MODULE);
    com_repository_.RegisterModule(internal_module);
    enum_module_filters(internal_module);
}

int32_t FactoryImplement::initialnize_engine(const std::string& root_dir, const std::string &declear_file, const FeatureMap &features){
    if(engine_){
        return -1;
    }

    root_dir_ = std::filesystem::path(root_dir).make_preferred() / "";

    MP_INFO("sdp engine use script root in: {}",root_dir_);


    auto engine = std::shared_ptr<Engine>(new Engine(root_dir_,declear_file));
    auto result = engine->init();
    if(result < 0)
        return -2;
    engine_ = engine;
    features_ = features;
    return 0;
}

std::shared_ptr<Engine> FactoryImplement::engine()
{
    return engine_;
}


IUnknownPointer FactoryImplement::create_object(const TGUID &clsid)
{
    return com_repository_.CreateObject(clsid);
}

const FilterDelear *FactoryImplement::filter_declear(const TGUID &clsid)
{
    if(filter_declears_.find(clsid) == filter_declears_.end())
        return nullptr;
    return &filter_declears_[clsid];
}

FilterPointer FactoryImplement::create_filter(const std::string &module)
{
    MP_LOG_DEAULT("FactoryImplement::create_filter {}", module.data());
    FilterPointer filter;

    for(auto& item : filter_declears_){
        if(item.second.module == module){
            auto object = com_repository_.CreateObject(item.first);
            object.QueryInterface(&filter);
            break;
        }
    }
    return filter;
}

const std::string &FactoryImplement::script_dir()
{    
    return root_dir_;
}

const FeatureMap &FactoryImplement::features()
{
    return features_;
}

int32_t FactoryImplement::enum_module_filters(tinycom::IComModule *module)
{
    int index = 0;
    const char* metadata = nullptr;
    while ((metadata = module->GetMetadata(index++)) != nullptr) {
        try{
            nlohmann::json filter_meta = nlohmann::json::parse(metadata);
            if(filter_meta.is_object()){
                if(!filter_meta.contains("type"))
                    continue;
                if(filter_meta["type"] != "sdp-filter")
                    continue;

                FilterDelear declear;
                declear.clsid = tinycom::TGUID(filter_meta["clsid"].get<std::string>().c_str());
                declear.describe = filter_meta["describe"].get<std::string>();
                declear.module = filter_meta["name"].get<std::string>();

                int type = 0;
                auto &types = filter_meta["filtertype"];
                for(auto& item : types){
                    type |= filter_type_from_string(item.get<std::string>().c_str());
                }
                declear.type = static_cast<FilterType>(type);

                auto &props = filter_meta["properties"];
                for(auto& item : props){
                    FilterProperty prop;
                    prop.name = item["name"];
                    std::string type = item["type"];
                    if(type == "number"){
                        prop.type = PropertyType::kPorpertyNumber;
                        prop.value = item["value"].get<double>();
                    }
                    else if(type == "string"){
                        prop.type = PropertyType::kPorpertyString;
                        prop.value = item["value"].get<std::string>();
                    }
                    else if(type == "bool"){
                        prop.type = PropertyType::kPorpertyBool;
                        prop.value = item["value"].get<bool>();
                    }
                    else if(type == "number_array"){
                        prop.type = PropertyType::kPorpertyNumberArray;
                        prop.value = item["value"].get<std::vector<double>>();
                    }
                    else if(type == "string_array"){
                        prop.type = PropertyType::kPorpertyStringArray;
                        prop.value = item["value"].get<std::vector<std::string>>();;
                    }
                    else if(type == "pointer"){
                        prop.type = PropertyType::kPorpertyPointer;
                        prop.value = item["value"].get<uint64_t>();
                    }
                    else if(type == "lua_function"){
                        prop.type = PropertyType::kPorpertyLuaFunction;
                        prop.value = sol::function();
                    }
                    else if(type == "lua_table"){
                        prop.type = PropertyType::kPorpertyLuaTable;
                        prop.value = sol::table();
                    }
                    else{
                        MP_WARN("unknown property type: {}#{}",declear.module,prop.name);
                        continue;
                    }
                    declear.properties.push_back(prop);
                }
                filter_declears_[declear.clsid] = declear;
                MP_INFO("Enum objects found filter:{} metainfo:\n{}",declear.module,metadata);
            }

        }
        catch (const nlohmann::json::parse_error&)
        {

        }
    }
    return 0;
}

const char* FactoryImplement::object_metadata(const TGUID &clsid) {
    //TODO
    return "";
}


}
