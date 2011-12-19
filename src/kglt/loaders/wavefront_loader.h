#ifndef WAVEFRONT_LOADER_H_INCLUDED
#define WAVEFRONT_LOADER_H_INCLUDED

#include "../loader.h"

namespace kglt {

class WavefrontLoader : public Loader {
public:
    void load_into(Loadable& resource, const std::string& filename);


};

}


#endif // WAVEFRONT_LOADER_H_INCLUDED
