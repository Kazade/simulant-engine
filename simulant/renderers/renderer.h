/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <vector>
#include <memory>

#include "../types.h"
#include "../generic/auto_weakptr.h"
#include "../window.h"

#include "batching/renderable.h"
#include "batching/render_queue.h"

namespace smlt {

class SubActor;
class HardwareBufferManager;

struct ElementRenderSpecification {
    MaterialID material_id;
    uint32_t count;
    uint8_t* indices; // Must be an array of UNSIGNED_SHORT
};

typedef std::vector<ElementRenderSpecification> ElementRenderList;

class Renderer:
    public batcher::RenderGroupFactory {

public:
    typedef std::shared_ptr<Renderer> ptr;

    Renderer(Window* window):
        window_(window) {}

    virtual std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera) = 0;

    Property<Renderer, Window> window = { this, &Renderer::window_ };

    virtual void init_context() = 0;
    // virtual void upload_texture(Texture* texture) = 0;

    Property<Renderer, HardwareBufferManager> hardware_buffers = { this, [](Renderer* self) {
        return self->_get_buffer_manager();
    }};

    virtual GPUProgramID new_or_existing_gpu_program(const std::string& vertex_shader, const std::string& fragment_shader) {
        return GPUProgramID();
    }

    virtual GPUProgramPtr gpu_program(GPUProgramID) const { return GPUProgramPtr(); }

public:
    // Render support flags
    virtual bool supports_gpu_programs() const { return false; }

    virtual HardwareBufferManager* _get_buffer_manager() const = 0;

    void register_texture(TextureID tex_id, TexturePtr texture) {
        std::lock_guard<std::recursive_mutex> lock(texture_registry_mutex_);
        if(!is_texture_registered(tex_id)) {
            on_texture_register(tex_id, texture);
            texture_registry_.insert(std::make_pair(tex_id, std::weak_ptr<Texture>(texture)));
        }
    }

    void unregister_texture(TextureID texture_id) {
        std::lock_guard<std::recursive_mutex> lock(texture_registry_mutex_);
        if(texture_registry_.count(texture_id)) {
            texture_registry_.erase(texture_id);
            on_texture_unregister(texture_id);
        }
    }

    /*
     * Returns true if the texture has been allocated, false otherwise.
     *
     * Should be thread-safe along with allocate/deallocate texture.
     */
    bool is_texture_registered(TextureID texture_id) const {
        std::lock_guard<std::recursive_mutex> lock(texture_registry_mutex_);
        return texture_registry_.count(texture_id);
    }

    void prepare_texture(TextureID texture_id) {
        std::lock_guard<std::recursive_mutex> lock(texture_registry_mutex_);

        if(!texture_registry_.count(texture_id)) {
            return;
        }

        auto tex = texture_registry_.at(texture_id).lock();
        if(tex) {
            on_texture_prepare(tex);
        }
    }

private:    
    Window* window_ = nullptr;

    /*
     * Called when a texture is created. This should do whatever is necessary to
     * prepare a texture for later upload
     *
     * It's likely that data will need to also be stored here for prepare_texture
     * to function
     *
     * This will be called when a render group (which uses textures) is created, which
     * means it can be called from any thread and should be thread-safe.
     */
    virtual void on_texture_register(TextureID tex_id, TexturePtr texture) {}

    /*
     * Called when a texture is destroyed, should perform any cleanup from
     * register_texture.
     *
     * This will be called when all render groups sharing the texture are destroyed
     * and so can be called from any thread and should be thread-safe
     */
    virtual void on_texture_unregister(TextureID tex_id) {}

    /*
     * Given a Texture, this should take care of:
     * - Uploading the texture if the data is dirty
     * - Updating texture filters and wrap modes if necessary
     * - Generating or deleting mipmaps if the mipmap generation changed
     *
     * Guaranteed to be called from the main (render) thread, although must obviously
     * be aware of register/unregister
     */
    virtual void on_texture_prepare(TexturePtr texture) {}

    mutable std::recursive_mutex texture_registry_mutex_; // Recursive because of nested is_texture_registered check
    std::unordered_map<TextureID, std::weak_ptr<Texture>> texture_registry_;
};

}

#endif // RENDERER_H
