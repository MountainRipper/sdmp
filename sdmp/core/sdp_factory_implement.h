#ifndef FACTORYIMPLEMENT_H
#define FACTORYIMPLEMENT_H
#include "core_includes.h"
#include "sdpi_factory.h"
#include "sdp_engine.h"

namespace sdp {

class FactoryImplement{
public:
    FactoryImplement();
    int32_t initialnize_engine(const std::string &root_dir,
                               const std::string& declear_file,
                               const FeatureMap& features);
    std::shared_ptr<Engine> engine();

    const char* object_metadata(const TGUID& clsid);
    IUnknownPointer create_object(const TGUID& clsid);

    const FilterDelear *filter_declear(const TGUID& clsid);
    FilterPointer create_filter(const std::string & module);

    const std::string& script_dir();
    const FeatureMap& features();
private:
    void extracted(tinycom::IComModule *&module, int &index,
                   const char *&metadata);
    int32_t enum_module_filters(tinycom::IComModule *module);

public:
    std::string root_dir_;
    tinycom::ComRepository com_repository_;
    std::map<tinycom::TGUID,FilterDelear> filter_declears_;
    FeatureMap features_;
    std::shared_ptr<Engine> engine_;
};

}


#endif // FACTORYIMPLEMENT_H
