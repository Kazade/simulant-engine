#pragma once

#include "../application.h"
#include "../color.h"
#include "../generic/any/any.h"
#include "../generic/optional.h"
#include "../loader.h"
#include "../nodes/actor.h"
#include "../nodes/camera.h"
#include "../nodes/light.h"
#include "../nodes/stage_node.h"
#include "../scenes/scene.h"
#include "../stage.h"
#include "../utils/json.h"
#include "../utils/limited_string.h"
#include "../utils/params.h"

#include <map>
#include <string>

namespace smlt {
namespace loaders {

class GLTFLoader: public smlt::Loader {
public:
    typedef std::map<std::string, smlt::any> CustomAttributeMap;

    struct CameraInfo {
        std::string type;
        float aspect;
        float yfov;
        float znear;
        smlt::optional<float> zfar;
    };

    struct LightInfo {
        std::string type;
        smlt::Color color = smlt::Color::white();
        float intensity = 1.0f;
        float range = 0.0f;
    };

    struct NodeFactoryInput {
        smlt::LimitedString<32> name;
        smlt::MeshPtr mesh;
        smlt::optional<CameraInfo> camera;
        smlt::optional<LightInfo> light;
        smlt::Vec3 translation;
        smlt::Quaternion rotation;
        smlt::Vec3 scale;
        smlt::Params params;
        CustomAttributeMap attrs;
    };

    typedef std::function<smlt::StageNode*(smlt::StageNode* parent,
                                           const NodeFactoryInput& attributes)>
        NodeFactory;

    static const char* node_factory_key;

    GLTFLoader(const smlt::Path& filename, std::shared_ptr<std::istream> data) :
        Loader(filename, data) {}

    /**
     * @brief Loads a gltf file into a Scene.
     *
     * @param resource
     * @param options
     *   node_factory: NodeFactory - if provided this will be called to
     instantiate each node. A map of custom attributes will be provided which is
     read from the "extras" key of the node, or its mesh (if any)
     */

    bool into(smlt::Loadable& resource, const smlt::LoaderOptions& options =
                                            smlt::LoaderOptions()) override;
};

class GLTFLoaderType: public smlt::LoaderType {
public:
    virtual ~GLTFLoaderType() {}

    const char* name() override {
        return "gltf";
    }

    bool supports(const smlt::Path& filename) const override {
        return filename.ext() == ".gltf" || filename.ext() == ".glb";
    }

    smlt::Loader::ptr
        loader_for(const smlt::Path& filename,
                   std::shared_ptr<std::istream> data) const override {
        return std::make_shared<GLTFLoader>(filename, data);
    }
};

} // namespace loaders
} // namespace smlt
