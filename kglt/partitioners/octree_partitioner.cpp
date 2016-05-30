#include "octree_partitioner.h"

#include "../stage.h"
#include "../light.h"
#include "../actor.h"
#include "../camera.h"
#include "../particles.h"
#include "../geom.h"

#include "./static_chunk.h"

/*
 * TODO:
 *
 * The tree stores a raw pointer to the boundable entity - surely this needs to be refcounted? What if the
 * entity is destroyed in another thread? There's so much to fix in this rendering system :/
 */

namespace kglt {

const std::string STATIC_CHUNK_KEY = "static_chunks";

void OctreePartitioner::event_actor_changed(ActorID ent) {
    L_DEBUG("Actor changed, updating partitioner");
    remove_actor(ent);
    add_actor(ent);
}

void OctreePartitioner::add_particle_system(ParticleSystemID ps) {
    auto system = stage->particle_system(ps); //Get a handle on the particle system

    BoundableEntity* ent = system.__object.get(); //Get the raw pointer as a BoundableEntity (bad?)
    tree_.grow(ent);
}

void OctreePartitioner::remove_particle_system(ParticleSystemID ps) {
    auto system = stage->particle_system(ps);
    BoundableEntity* ent = system.__object.get(); //Get the raw pointer as a BoundableEntity (bad?)
    tree_.shrink(ent);
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

    GeomPtr geom = stage->geom(geom_id);
    auto geom_mesh = stage->mesh(geom->mesh_id());

    // Go through all the submeshes, separating into polygons and adding to the tree
    geom_mesh->each([&](SubMesh* submesh) {
        auto material_id = submesh->material_id();
        auto render_priority = geom->render_priority();
        auto mesh_arrangement = submesh->arrangement();

        std::vector<Polygon> polygons;

        switch(submesh->arrangement()) {
            case kglt::MESH_ARRANGEMENT_TRIANGLES: {
                assert(submesh->index_data().count() % 3 == 0);

                uint32_t j = 0;
                polygons.resize(submesh->index_data().count() / 3);
                for(uint32_t i = 0; i < submesh->index_data().count(); i+=3) {
                    Polygon& poly = polygons[j++];

                    poly.source_data = &submesh->vertex_data();
                    poly.indices.push_back(submesh->index_data().at(i));
                    poly.indices.push_back(submesh->index_data().at(i+1));
                    poly.indices.push_back(submesh->index_data().at(i+2));
                }

            } break;
            default: {
                // Just insert a bulk thing. Not efficient!
                polygons.resize(1);
                auto& poly = polygons.front();

                poly.source_data = &submesh->vertex_data();
                auto& data = submesh->index_data().all();
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
                    subchunk->vertex_data().inherit_enabled_bitmask(submesh->vertex_data());
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

    std::cout << "Subchunk count: " << num_created << std::endl;
}

void OctreePartitioner::remove_geom(GeomID geom_id) {
    /* FIXME: Need to be able to remove the geom, but also signal to the render queue to remove all subchunks */
    assert(0 && "Not Implemented");
}

void OctreePartitioner::add_actor(ActorID obj) {
    L_DEBUG("Adding actor to the partitioner");

    auto ent = stage->actor(obj);
    for(uint16_t i = 0; i < ent->subactor_count(); ++i) {
        //All subactors are boundable
        BoundableEntity* boundable = &ent->subactor(i);
        tree_.grow(boundable);

        actor_to_registered_subactors_[obj].push_back(boundable);
        boundable_to_renderable_[boundable] = ent->_subactors().at(i);
    }

    //Connect the changed signal
    actor_changed_connections_[obj] = ent->signal_mesh_changed().connect(
        std::bind(&OctreePartitioner::event_actor_changed, this, std::placeholders::_1)
    );
}

void OctreePartitioner::remove_actor(ActorID obj) {
    L_DEBUG("Removing actor from the partitioner");

    //Remove all boundable subactors that were linked to the actor
    for(BoundableEntity* boundable: actor_to_registered_subactors_[obj]) {
        tree_.shrink(boundable);
        boundable_to_renderable_.erase(boundable);
    }

    //Erase the list of subactors linked to this actor
    actor_to_registered_subactors_.erase(obj);

    //Disconnect the changed signal
    actor_changed_connections_[obj].disconnect();
    actor_changed_connections_.erase(obj);
}

void OctreePartitioner::add_light(LightID obj) {
    //FIXME: THis is nasty and dangerous
    auto light = stage->light(obj);
    BoundableEntity* boundable = light.get();
    assert(boundable);
    tree_.grow(boundable);
    boundable_to_light_[boundable] = obj;
}

void OctreePartitioner::remove_light(LightID obj) {
    auto light = stage->light(obj);
    BoundableEntity* boundable = light.get();
    assert(boundable);
    tree_.shrink(boundable);
    boundable_to_light_.erase(boundable);
}

std::vector<RenderablePtr> OctreePartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<RenderablePtr> results;

    //If the tree has no root then we return nothing
    if(!tree_.has_root()) {
        return results;
    }

    /**
     *  FIXME: A tree_->objects_visible_from(cam.frustum()); would be faster
     */

    //Go through the visible nodes
    for(OctreeNode* node: tree_.nodes_visible_from(stage->window->camera(camera_id)->frustum())) {
        //Go through the objects
        for(const BoundableEntity* obj: node->objects()) {
            if(container::contains(boundable_to_renderable_, obj)) {
                //Build a list of visible subactors
                results.push_back(boundable_to_renderable_[obj]);
            }
        }

        if(node->exists(STATIC_CHUNK_KEY)) {
            auto static_chunks = node->get<StaticChunkHolder::ptr>(STATIC_CHUNK_KEY);
            for(auto& chunk: static_chunks->chunks) {
                auto& static_chunk = chunk.second;
                for(auto& subchunk: *static_chunk->subchunks.get()) {
                    results.push_back(subchunk.second);
                }
            }
        }
    }

    return results;
}

std::vector<LightID> OctreePartitioner::lights_visible_from(CameraID camera_id) {
    std::vector<LightID> lights;
    return lights;
}

}
