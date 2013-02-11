#include "../kazbase/exceptions.h"
#include "../kazbase/string.h"
#include "../kazbase/logging.h"
#include "../kazbase/unicode.h"
#include "../mesh.h"
#include "dxf_loader.h"

namespace kglt {
namespace loaders {

void DXFLoader::into(Loadable& resource) {
    kglt::Mesh* tmp = dynamic_cast<Mesh*>(&resource);
    if(!tmp) {
        throw IOError("Attempted to load a DXF file into an incompatible resource");
    }

    kglt::Mesh& mesh = *tmp;

    std::ifstream file(filename_.c_str(), std::ios::binary);
    if(!file.good()) {
        throw IOError("Couldn't load the DXF file: " + filename_);
    }

    std::vector<kmVec3> vertices;

    std::string line;

    bool building_polyline = false;
    bool building_vertex = false;
    kmVec3 current_vertex;

    while(std::getline(file, line)) {        
        line = str::strip(line, " \r\n");
        int group_code = boost::lexical_cast<int>(line);
        std::string identifier;
        getline(file, identifier);
        identifier = str::strip(identifier, " \r\n");

        if(group_code < 10) {
            if(identifier == "SECTION") {

            } else if(identifier == "POLYLINE") {
                building_polyline = true;
            } else if(identifier == "VERTEX") {
                if(!building_polyline) {
                    L_DEBUG("Found vertex outside POLYLINE");
                    continue;
                }
                //We are done building this vertex
                if(building_vertex) {
                    vertices.push_back(current_vertex);
                }

                //Start the next vertex
                building_vertex = true;
            } else if(identifier == "SEQEND") {
                building_polyline = false;
                building_vertex = false;
            } else {
                L_DEBUG("Unhandled string: " + identifier);
            }

        } else if(group_code == 10 || group_code == 20 || group_code == 30) {
            if(building_vertex) {
                float value = boost::lexical_cast<double>(identifier);
                switch(group_code) {
                    case 10: current_vertex.x = value;
                    break;
                    case 20: current_vertex.y = value;
                    break;
                    case 30: current_vertex.z = value;
                    break;
                    default:
                        assert(false); //shouldn't happen!
                }
            }
        } else if(group_code == 999) {
            continue; //comment
        }
    }

    L_DEBUG(unicode("Loaded {0} vertices").format(vertices.size()).encode());
}

}
}
