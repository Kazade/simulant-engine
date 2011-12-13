#ifndef Q2BSP_LOADER_H_INCLUDED
#define Q2BSP_LOADER_H_INCLUDED

#include "../loader.h"

namespace GL {
namespace loaders {

class Q2BSPLoader : public Loader {
public:
    void load_into(Resource& resource, const std::string& filename);

};

}
}


#endif // Q2BSP_LOADER_H_INCLUDED
