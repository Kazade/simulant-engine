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

    /*
     * Called when a texture is created. This should do whatever is necessary to
     * prepare a texture for later upload (in GL this would call glGenTextures
     *
     * It's likely that data will need to also be stored here for prepare_texture
     * to function (e.g. the resulting GLuint)
     *
     * This will be called from the main (rendering) thread.
     */
    virtual void allocate_texture(TextureID tex_id, TexturePtr texture) = 0;

    /*
     * Called when a texture is destroyed, should perform any cleanup from
     * allocate_texture.
     *
     * This will be called from the main (rendering) thread.
     */
    virtual void deallocate_texture(TextureID texture_id) = 0;

    /*
     * Returns true if the texture has been allocated, false otherwise
     */
    virtual bool was_texture_allocated(TextureID texture_id) const = 0;

    /*
     * Given a Texture, this should take care of:
     * - Uploading the texture if the data is dirty
     * - Updating texture filters and wrap modes if necessary
     * - Generating or deleting mipmaps if the mipmap generation changed
     */
    virtual void prepare_texture(TexturePtr texture) = 0;

private:    
    Window* window_ = nullptr;

};

}

#endif // RENDERER_H
