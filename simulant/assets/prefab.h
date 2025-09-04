#pragma once

#include <cstdint>
#include <vector>

#include "../asset.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../types.h"
#include "../utils/params.h"

namespace smlt {

class GLTFLoader;

struct PrefabNode {
    uint32_t id;
    LimitedString<64> node_type_name;
    Params params;

    Vec3 translation;
    Quaternion rotation;
    Vec3 scale;
};

/**
 * @brief The Prefab class
 *
 * A Prefab is a template for a heirarchy of stage nodes.
 *
 * It can be used to load anything from entire scenes to animated
 * meshes into the scene tree.
 *
 * Once loaded, a `Prefab` can be added to a scene using
 * a `PrefabInstance`.
 */
class Prefab:
    public Asset,
    public Loadable,
    public RefCounted<Prefab>,
    public generic::Identifiable<AssetID> {

public:
    Prefab(AssetID id, AssetManager* asset_manager) :
        Asset(asset_manager), generic::Identifiable<AssetID>(id) {}

    void push_node(PrefabNode node, int32_t parent_id = -1);

    void push_texture(TexturePtr texture) {
        textures_.push_back(texture);
    }

    void push_material(MaterialPtr material) {
        materials_.push_back(material);
    }

    void push_mesh(MeshPtr mesh) {
        meshes_.push_back(mesh);
    }

private:
    std::vector<TexturePtr> textures_;
    std::vector<MaterialPtr> materials_;
    std::vector<MeshPtr> meshes_;

    friend class GLTFLoader;
};

} // namespace smlt
