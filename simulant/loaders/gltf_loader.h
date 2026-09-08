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

    /* One entry of the SMLT_scene_script extension on a gltf scene's
     * "extensions" object:
     *
     *   "extensions": {
     *     "SMLT_scene_script": {
     *       "scripts": [
     *         {"uri": "scene.lua", "language": "lua", "type": "scene", "class": "MyScene"},
     *         {"uri": "enemy.lua", "language": "lua", "type": "stage_node", "class": "Enemy"}
     *       ]
     *     }
     *   }
     *
     * "uri" may be an external path (resolved relative to the gltf file's
     * own folder) or a base64 data: URI; either way `source` holds the
     * fully-resolved script text after find_scene_scripts() returns.
     */
    struct SceneScriptDef {
        std::string language;
        std::string type; // "scene" or "stage_node"
        std::string class_name;
        std::string source;
    };

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

    /**
     * @brief Reads the SMLT_scene_script extension (if any) from this gltf
     * file's default scene, resolving each script's source text.
     *
     * Returns false (with an error logged) if the file can't be read/parsed
     * or an entry is malformed. An empty `out` is a valid "no scripts"
     * result — it is not an error for a gltf file to have no scripts.
     */
    bool find_scene_scripts(std::vector<SceneScriptDef>& out);
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
