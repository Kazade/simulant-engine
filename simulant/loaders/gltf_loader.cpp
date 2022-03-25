#include "gltf_loader.h"

#include "../utils/json.h"

namespace smlt {
namespace loaders {


void parse_mesh_node(Mesh* target, JSONNode& json) {

}


void GLTFLoader::into(Loadable &resource, const LoaderOptions &options) {
    bool is_binary = filename_.ext() == ".glb";
    bool is_scene = bool(loadable_to<Stage>(resource));

    if(is_binary) {
        struct Header {
            uint32_t magic;
            uint32_t version;
            uint32_t length;
        } header;

        data_.read((char*) &header, sizeof(Header));

        if(header.magic != 0x46546C67 || header.version < 2) {
            S_ERROR("Unable to load invalid GLTF file");
        }

    } else {

    }
}


}
}
