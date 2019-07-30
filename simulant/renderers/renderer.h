/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <vector>
#include <memory>

#include "../types.h"
#include "../generic/threading/shared_mutex.h"
#include "../window.h"

#include "batching/renderable.h"
#include "batching/render_queue.h"

namespace smlt {

class SubActor;

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

    virtual GPUProgramID new_or_existing_gpu_program(const std::string& vertex_shader, const std::string& fragment_shader) {
        return GPUProgramID();
    }

    virtual GPUProgramID current_gpu_program_id() const { return GPUProgramID(); }
    virtual GPUProgramPtr gpu_program(GPUProgramID) const { return GPUProgramPtr(); }
    virtual GPUProgramID default_gpu_program_id() const { return GPUProgramID(); }

    virtual std::string name() const = 0;

    /* This function is called just before drawing the renderable, it can be
     * used to upload any data to VRAM if necessary */
    virtual void prepare_to_render(Renderable* renderable) = 0;
public:
    // Render support flags
    virtual bool supports_gpu_programs() const { return false; }

    void register_texture(TextureID tex_id, TexturePtr texture);

    void unregister_texture(TextureID texture_id);

    /*
     * Returns true if the texture has been allocated, false otherwise.
     *
     * Should be thread-safe along with allocate/deallocate texture.
     */
    bool is_texture_registered(TextureID texture_id) const;

    void prepare_texture(TextureID texture_id);

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
     * Called when a texture is destroyed, should perform any clean_up from
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

    mutable shared_mutex texture_registry_mutex_;
    std::unordered_map<TextureID, std::weak_ptr<Texture>> texture_registry_;
};

}

#endif // RENDERER_H
