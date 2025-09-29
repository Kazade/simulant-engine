#pragma once

#include <cstdint>
#include <list>
#include <vector>

#include "../asset.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../interfaces/nameable.h"
#include "../nodes/animation_controller.h"
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
};

struct PrefabAnimationChannel {
    PrefabNode target;
    AnimationPath path;
    AnimationInterpolation interpolation;
    AnimationDataPtr data;
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

    std::size_t node_count() const {
        return nodes_.size();
    }

    template<typename Callback>
    void each_node(const Callback& cb) const {
        for(auto& node: nodes_) {
            cb(node.first, node.second);
        }
    }

    bool has_animations() const {
        return !animations_.empty();
    }

    std::size_t animation_count() const {
        std::size_t c = 0;
        for(auto it = animations_.begin(); it != animations_.end();
            it = animations_.upper_bound(it->first)) {
            ++c;
        }
        return c;
    }

    void push_animation_channel(const std::string& animation, PrefabNode node,
                                AnimationPath path, AnimationInterpolation i,
                                AnimationDataPtr data) {
        PrefabAnimationChannel channel;
        channel.data = data;
        channel.interpolation = i;
        channel.path = path;
        channel.target = node;

        animations_.insert(std::make_pair(animation, channel));
    }

    template<typename Callback>
    void each_animation(const Callback& cb) const {
        std::string anim_name;
        std::vector<PrefabAnimationChannel> channels;
        for(auto& animation: animations_) {
            if(!anim_name.empty() && anim_name != animation.first) {
                cb(anim_name, channels);
                channels.clear();
            } else {
                anim_name = animation.first;
                channels.push_back(animation.second);
            }
        }

        if(!channels.empty()) {
            cb(anim_name, channels);
        }
    }

private:
    std::multimap<PrefabKey, PrefabNode> nodes_;
    std::multimap<std::string, PrefabAnimationChannel> animations_;

    std::vector<TexturePtr> textures_;
    std::vector<MaterialPtr> materials_;
    std::vector<MeshPtr> meshes_;

    friend class GLTFLoader;
};

} // namespace smlt
