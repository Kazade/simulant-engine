//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "octree_partitioner.h"

#include "../stage.h"
#include "../light.h"
#include "../actor.h"
#include "../camera.h"
#include "../particles.h"
#include "../geom.h"

namespace smlt {

bool should_split_predicate(const octree_impl::OctreeNode* node) {
    return node->data->actor_count() + node->data->particle_system_count() > 30;
}

bool should_merge_predicate(const octree_impl::NodeList& nodes) {
    uint32_t total = 0;
    for(auto& node: nodes) {
        total += node.lock()->data->actor_count();
        total += node.lock()->data->particle_system_count();
    }

    return total < 20;
}



const std::string STATIC_CHUNK_KEY = "static_chunks";

void OctreePartitioner::event_actor_changed(ActorID ent) {
    L_DEBUG("Actor changed, updating partitioner");
    remove_actor(ent);
    add_actor(ent);
}

void OctreePartitioner::add_particle_system(ParticleSystemID ps) {
    tree_.insert_particle_system(ps);
}

void OctreePartitioner::remove_particle_system(ParticleSystemID ps) {
    tree_.remove_particle_system(ps);
}

void OctreePartitioner::add_geom(GeomID geom_id) {
    uint32_t num_created = 0;
    /*
     * 1. Iterate through the polygons in the geom, for each one generate a BoundableEntity
     * 2. Insert into the tree, but set a callback for grow()
     * 3. The callback must stash a new static chunk (if it doesn't exist on the node) otherwise
     *    get a handle to the existing one.
     * 4. Add the polygon to the static chunk, return false from the callback so that the octree
     *    doesn't add the polygon to the object list on the node
     */

    /*
    GeomPtr geom = stage->geom(geom_id);
    auto geom_mesh = stage->assets->mesh(geom->mesh_id());

    // Go through all the submeshes, separating into polygons and adding to the tree
    geom_mesh->each([&](SubMesh* submesh) {
        auto material_id = submesh->material_id();
        auto render_priority = geom->render_priority();
        auto mesh_arrangement = submesh->arrangement();

        std::vector<Polygon> polygons;

        switch(submesh->arrangement()) {
            case smlt::MESH_ARRANGEMENT_TRIANGLES: {
                assert(submesh->index_data->count() % 3 == 0);

                uint32_t j = 0;
                polygons.resize(submesh->index_data->count() / 3);
                for(uint32_t i = 0; i < submesh->index_data->count(); i+=3) {
                    Polygon& poly = polygons[j++];

                    poly.source_data = &submesh->vertex_data();
                    poly.indices.push_back(submesh->index_data->at(i));
                    poly.indices.push_back(submesh->index_data->at(i+1));
                    poly.indices.push_back(submesh->index_data->at(i+2));
                }

            } break;
            default: {
                // Just insert a bulk thing. Not efficient!
                polygons.resize(1);
                auto& poly = polygons.front();

                poly.source_data = &submesh->vertex_data();
                auto& data = submesh->index_data->all();
                poly.indices.assign(data.begin(), data.end());
            }
        }

        for(auto& polygon: polygons) {
            polygon.recalc_bounds();

            tree_.grow(&polygon, [=, &num_created](const BoundableEntity* ent, OctreeNode* node) -> bool {
                StaticChunkHolder::ptr static_chunks;

                if(!node->exists(STATIC_CHUNK_KEY)) {
                    static_chunks.reset(new StaticChunkHolder());
                    node->stash(static_chunks, STATIC_CHUNK_KEY);
                    ++num_created;
                } else {
                    static_chunks = node->get<StaticChunkHolder::ptr>(STATIC_CHUNK_KEY);
                }

                // If there isn't a chunk for this geom, create one
                if(!static_chunks->chunks.count(geom_id)) {
                    static_chunks->chunks.insert(std::make_pair(geom_id, std::make_shared<StaticChunk>(stage.get())));
                    signal_static_chunk_created_(static_chunks->chunks.at(geom_id).get());
                }

                // Get the static chunk for this geom
                auto& static_chunk = static_chunks->chunks.at(geom_id);

                // Create a subchunk for this polygon if necessary
                auto subchunk_and_result = static_chunk->get_or_create_subchunk(std::make_tuple(render_priority, material_id, mesh_arrangement));

                auto subchunk = subchunk_and_result.first;
                auto created = subchunk_and_result.second;

                if(created) {
                    // We need to make sure the same vertex attributes are enabled as they were on the source submesh
                    subchunk->vertex_data->inherit_enabled_bitmask(submesh->vertex_data());
                    StaticChunkChangeEvent evt;
                    evt.type = STATIC_CHUNK_CHANGE_TYPE_SUBCHUNK_CREATED;
                    evt.subchunk_created.subchunk = subchunk;
                    signal_static_chunk_changed_(static_chunk.get(), evt);
                }

                // Add the polygon to the subchunk
                subchunk->add_polygon(polygon);

                return false; // Don't do the default tree insertion behaviour, we just overrode it
            });
        }
    });

    std::cout << "Subchunk count: " << num_created << std::endl; */
}

void OctreePartitioner::remove_geom(GeomID geom_id) {
    /* FIXME: Need to be able to remove the geom, but also signal to the render queue to remove all subchunks */
    assert(0 && "Not Implemented");
}

void OctreePartitioner::add_actor(ActorID obj) {
    L_DEBUG("Adding actor to the partitioner");

    if(stage->actor(obj)->renderable_culling_mode() == RENDERABLE_CULLING_MODE_NEVER) {
        actors_always_visible_.insert(obj);
    } else {
        tree_.insert_actor(obj);
    }
}

void OctreePartitioner::remove_actor(ActorID obj) {
    L_DEBUG("Removing actor from the partitioner");

    if(actors_always_visible_.count(obj)) {
        actors_always_visible_.erase(obj);
    } else {
        tree_.remove_actor(obj);
    }
}

void OctreePartitioner::add_light(LightID obj) {
    if(stage->light(obj)->renderable_culling_mode() == RENDERABLE_CULLING_MODE_NEVER) {
        lights_always_visible_.insert(obj);
    } else {
        tree_.insert_light(obj);
    }
}

void OctreePartitioner::remove_light(LightID obj) {
    if(lights_always_visible_.count(obj)) {
        lights_always_visible_.erase(obj);
    } else {
        tree_.remove_light(obj);
    }
}

std::vector<RenderablePtr> OctreePartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<RenderablePtr> results;

    /* Make sure we return all actors which are always visible regardless of culling */
    for(auto& aid: actors_always_visible_) {
        auto actor = stage->actor(aid);
        for(auto subactor: actor->_subactors()) {
            results.push_back(subactor);
        }
    }

    //If the tree has no root then there's nothing more to add
    if(!tree_.has_root()) {
        return results;
    }

    auto camera = stage->window->camera(camera_id);
    auto& frustum = camera->frustum();

    octree_impl::traverse(
        tree_,
        [&](octree_impl::OctreeNode* node) -> bool {
            if(frustum.intersects_aabb(node->loose_aabb())) {
                node->data->each_actor([&](ActorID actor_id, AABB aabb) {
                    auto actor = stage->actor(actor_id);
                    for(auto subactor: actor->_subactors()) {
                        results.push_back(subactor);
                    };
                });

                node->data->each_particle_system([&](ParticleSystemID ps_id, AABB aabb) {
                    auto system = stage->particle_system(ps_id);
                    results.push_back(system->shared_from_this());
                });

                return true;
            }

            return false;
        }
    );

    return results;
}

std::vector<LightID> OctreePartitioner::lights_visible_from(CameraID camera_id) {
    std::vector<LightID> results(lights_always_visible_.begin(), lights_always_visible_.end());

    //If the tree has no root then we return nothing
    if(!tree_.has_root()) {
        return results;
    }

    auto camera = stage->window->camera(camera_id);
    auto& frustum = camera->frustum();

    octree_impl::traverse(
        tree_,
        [&](octree_impl::OctreeNode* node) -> bool {
            if(frustum.intersects_aabb(node->aabb())) {
                node->data->each_light([&](LightID light_id, AABB aabb) {
                    results.push_back(light_id);
                });
                return true;
            }

            return false;
        }
    );

    return results;
}

}
