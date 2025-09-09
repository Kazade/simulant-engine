#pragma once

#include <cstdint>
#include <list>
#include <vector>

#include "../asset.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../interfaces/nameable.h"
#include "../types.h"
#include "../utils/params.h"

namespace smlt {

class GLTFLoader;

struct PrefabKey {
    LimitedVector<uint32_t, 16> path;

    bool operator<(const PrefabKey& rhs) const {
        auto min =
            (path.size() < rhs.path.size()) ? path.size() : rhs.path.size();
        for(std::size_t i = 0; i < min; ++i) {
            if(path[i] < rhs.path[i]) {
                return true;
            }
        }

        return false;
    }
};

struct PrefabNode {
    uint32_t id;    
    LimitedString<64> node_type_name;
    Params params;

    LimitedString<Nameable::MAX_NAME_LENGTH> name;
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

    void push_node(PrefabNode node, int32_t parent_id = -1) {
        PrefabKey parent;

        // if there is a parent, find the existing node with the parent id
        // and build a path to the node
        if(parent_id != -1) {
            for(auto& p: nodes_) {
                if(p.first.path[p.first.path.size() - 1] ==
                   (uint32_t)parent_id) {
                    parent = p.first;
                }
            }
        }

        parent.path.push_back(node.id);
        nodes_.insert(std::make_pair(parent, node));
    }

    void push_texture(TexturePtr texture) {
        textures_.push_back(texture);
    }

    void push_material(MaterialPtr material) {
        materials_.push_back(material);
    }

    void push_mesh(MeshPtr mesh) {
        meshes_.push_back(mesh);
    }

    std::size_t node_count() const {
        return nodes_.size();
    }

    template<typename Callback>
    void each_node(const Callback& cb) const {
        for(auto& node: nodes_) {
            cb(node.first, node.second);
        }
    }

private:
    std::multimap<PrefabKey, PrefabNode> nodes_;

    std::vector<TexturePtr> textures_;
    std::vector<MaterialPtr> materials_;
    std::vector<MeshPtr> meshes_;

    friend class GLTFLoader;
};

} // namespace smlt
