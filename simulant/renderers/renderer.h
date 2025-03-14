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
#include "../threads/shared_mutex.h"
#include "../macros.h"
#include "../texture.h"

#include "batching/renderable.h"
#include "batching/render_queue.h"

namespace smlt {

class SubActor;
class Window;

class Renderer:
    public batcher::RenderGroupFactory {

public:
    typedef std::shared_ptr<Renderer> ptr;

    Renderer(Window* window):
        window_(window) {}

    virtual std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera) = 0;

    Property<Window* Renderer::*> window = { this, &Renderer::window_ };

    virtual void init_context() = 0;
    // virtual void upload_texture(Texture* texture) = 0;

    virtual GPUProgramPtr new_or_existing_gpu_program(const std::string& vertex_shader, const std::string& fragment_shader) {
        _S_UNUSED(vertex_shader);
        _S_UNUSED(fragment_shader);
        return GPUProgramPtr();
    }

    virtual GPUProgramPtr current_gpu_program() const { return GPUProgramPtr(); }
    virtual GPUProgramPtr gpu_program(const GPUProgramID&) const { return GPUProgramPtr(); }
    virtual GPUProgramPtr default_gpu_program() const { return GPUProgramPtr(); }

    virtual std::string name() const = 0;

    /* This function is called just before drawing the renderable, it can be
     * used to upload any data to VRAM if necessary */
    virtual void prepare_to_render(const Renderable* renderable) = 0;


    /** Returns true if the GPU can support the texture format
     *  without any kind of conversion */
    bool natively_supports_texture_format(TextureFormat fmt) {
        return texture_format_is_native(fmt);
    }

    /** Returns true if the renderer can convert the specified format
     * to make it usable, or if it natively supports the format */
    bool supports_texture_format(TextureFormat fmt) {
        if(natively_supports_texture_format(fmt)) {
            return true;
        }

        return texture_format_is_usable(fmt);
    }

    virtual void apply_viewport(const RenderTarget& target,
                                const Viewport& viewport) {
        _S_UNUSED(target);
        _S_UNUSED(viewport);
    }

    virtual void clear(const RenderTarget& target, const Color& color,
                       uint32_t clear_flags) {
        _S_UNUSED(target);
        _S_UNUSED(color);
        _S_UNUSED(clear_flags);
    }

    virtual void do_swap_buffers() {}

    virtual std::size_t max_texture_size() const {
        return 1024;
    }

public:
    /** To be overridden by subclasses. Default supported textures
     *  are those that are supported by glTexImage2D without any
     *  extensions */
    virtual bool texture_format_is_native(TextureFormat fmt);

    /** To be overridden by subclasses to specify texture formats that
     * can be handled by the renderer */
    virtual bool texture_format_is_usable(TextureFormat fmt);

    // Render support flags
    virtual bool supports_gpu_programs() const { return false; }

    /*
     * Returns true if the texture has been allocated, false otherwise.
     */
    bool is_texture_registered(AssetID texture_id) const;
    void pre_render();
    void post_render();

    void prepare_texture(Texture *texture);
    void prepare_material(Material* material);

private:
    friend class Texture;

    void register_texture(AssetID tex_id, Texture *texture);
    void unregister_texture(AssetID texture_id, Texture* texture);

    bool convert_if_necessary(Texture* tex);

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
    virtual void on_texture_register(AssetID tex_id, Texture* texture) {
        _S_UNUSED(tex_id);
        _S_UNUSED(texture);
    }

    /*
     * Called when a texture is destroyed, should perform any clean_up from
     * register_texture.
     *
     * This will be called when all render groups sharing the texture are destroyed
     * and so can be called from any thread and should be thread-safe
     */
    virtual void on_texture_unregister(AssetID tex_id, Texture* texture) {
        _S_UNUSED(tex_id);
        _S_UNUSED(texture);
    }

    /*
     * Given a Texture, this should take care of:
     * - Uploading the texture if the data is dirty
     * - Updating texture filters and wrap modes if necessary
     * - Generating or deleting mipmaps if the mipmap generation changed
     *
     * Guaranteed to be called from the main (render) thread, although must obviously
     * be aware of register/unregister
     */
    virtual void on_texture_prepare(Texture* texture) {
        _S_UNUSED(texture);
    }

    /* Called when a Material is created, or when it is changed via
     * an assignment. FIXME: This seems like we'd miss occasions where
     * we should call this. Might be better being called on first update? */
    virtual void on_material_prepare(Material* material) {
        _S_UNUSED(material);
    }

    /* Called at the start of pre_render() */
    virtual void on_pre_render() {}
    virtual void on_post_render() {}

    mutable thread::Mutex texture_registry_mutex_;
    std::unordered_map<AssetID, Texture*> texture_registry_;
};

}

#endif // RENDERER_H
