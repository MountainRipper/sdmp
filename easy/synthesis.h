#ifndef SYNTHESIS_H
#define SYNTHESIS_H
#include <cstdint>
#include <string>
#include "sdmpi_basic_types.h"
namespace mr {
namespace sdmp {

class SynthesisPrivateContex;
class Synthesis
{
public:
    Synthesis(const std::string &base_scipts_dir, const std::string &easy_scipts_dir);
    ~Synthesis();
private:
    SynthesisPrivateContex* context_ = nullptr;
};


}//namespace mr::sdmp
}//namespace mountain-ripper
#endif // SYNTHESIS_H
