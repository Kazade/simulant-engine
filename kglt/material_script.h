#ifndef MATERIAL_SCRIPT_H
#define MATERIAL_SCRIPT_H

#include <string>
#include "types.h"

namespace kglt {

class MaterialScript {
public:
    MaterialScript(Scene& scene, const std::string& path);
    MaterialID generate();

private:
    Scene& scene_;
    std::string filename_;
};

}

#endif // MATERIAL_SCRIPT_H
