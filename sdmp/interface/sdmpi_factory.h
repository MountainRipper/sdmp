#ifndef SDP_FACTORY_H
#define SDP_FACTORY_H
#include "sdmpi_graph.h"
// main factory for Script Describe Player
namespace mr::sdmp {
    class FactoryImplement;
    class Factory{
    public:
        static int32_t initialize_factory();
        static int32_t deinitialize();
        //create global engine for all graph. (such as audio output device...)
        //features must include:
        //graphicDriver:   egl,glx,wgl,agl,eagl,wayland,linuxfb,directfb
        static int32_t initialnize_engine(const std::string &root_dir,
                                          const std::string& declear_file,
                                          const FeatureMap& features);
        static const std::string& script_root();
        //create a graph for playback
        static std::shared_ptr<IGraph> create_graph_from(const std::string& declear_file,GraphEvent event);

        //create a stub objects
        static ISdpStub* create_stub(const std::string& class_name);

        static IUnknownPointer create_object(const TGUID& clsid);

        static const char* object_metadata(const TGUID& clsid);
        static const FilterDelear *filter_declear(const TGUID& clsid);

        static std::shared_ptr<FactoryImplement> factory();
    private:
        static std::shared_ptr<FactoryImplement> factory_;
    };

}


#endif // SDP_FACTORY_H
