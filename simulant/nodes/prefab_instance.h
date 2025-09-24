#pragma once

#include <map>

#include "../assets/prefab.h"
#include "light.h"
#include "stage_node.h"

namespace smlt {

class PrefabInstance: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_PREFAB_INSTANCE,
                             "prefab_instance");
    S_DEFINE_STAGE_NODE_PARAM(PrefabInstance, "prefab", PrefabPtr, no_value,
                              "The prefab to spawn");

    PrefabInstance(Scene* owner) :
        StageNode(owner, Meta::node_type) {}

    bool on_create(Params params) override {
        if(!clean_params<PrefabInstance>(params)) {
            return false;
        }

        auto prefab = params.get<PrefabRef>("prefab").value_or(PrefabPtr());
        auto prefab_ptr = prefab.lock();
        if(!prefab_ptr) {
            S_ERROR("Prefab was unexpectedly null");
            return false;
        }

        build_tree(prefab_ptr);
        return StageNode::on_create(params);
    }

private:
    void build_tree(const PrefabPtr& prefab) {
        std::map<uint32_t, StageNodePtr> node_map;

        auto cb = [&](const PrefabKey& key, const PrefabNode& node) {
            StageNodePtr parent = this;
            if(key.path.size() > 1) {
                // Has a parent
                parent = node_map.at(key.path[key.path.size() - 2]);
            }

            auto new_node = default_node_factory(parent, node);
            node_map.insert(std::make_pair(node.id, new_node));            
        };

        prefab->each_node(cb);
    }

    StageNode* default_node_factory(StageNode* parent,
                                    const PrefabNode& input) {
        StageNode* ret = nullptr;
        Light* light = nullptr;

        auto s_node = input.params.get<std::string>("s_node");

        auto node_type_name =
            s_node ? s_node.value() : input.node_type_name.str();

        /* If someone specifies the node type in the extras, we create the node
         * by name*/
        ret = parent->scene->create_node(node_type_name, input.params, nullptr);

        if(!ret) {
            S_WARN("Unable to spawn node with name: {0}", node_type_name);
        } else {
            ret->set_parent(parent);
        }

        if(ret) {
            ret->set_name(input.name);
            if(light && light->light_type() == LIGHT_TYPE_DIRECTIONAL) {
                light->set_direction(ret->transform->orientation().forward());
            }

            if(input.params.contains("s_mixins")) {
                auto mixins_maybe = input.params.get<std::string>("s_mixins");
                if(mixins_maybe) {
                    auto mixins = smlt::split(mixins_maybe.value(), ",");
                    for(std::size_t i = 0; i < mixins.size(); ++i) {
                        auto mixin_name = strip(mixins[i]);
                        Params mixin_params;
                        std::string prefix =
                            "s_mixin." + std::to_string(i) + ".";
                        for(auto arg_name: input.params.arg_names()) {
                            if(arg_name.str().find(prefix) == 0) {
                                auto key = arg_name.str().substr(prefix.size());
                                mixin_params.set(
                                    key.c_str(),
                                    input.params.raw(arg_name.c_str()).value());
                            }
                        }

                        if(input.params.contains("mesh")) {
                            mixin_params.set(
                                "mesh",
                                input.params.get<MeshRef>("mesh").value_or(
                                    MeshPtr()));
                        }

                        auto new_mixin =
                            ret->create_mixin(mixin_name, mixin_params);
                        if(!new_mixin) {
                            S_ERROR("Failed to create mixin with name {0}",
                                    mixin_name);
                        }
                    }
                }
            }
        }

        return ret;
    }
};

} // namespace smlt
