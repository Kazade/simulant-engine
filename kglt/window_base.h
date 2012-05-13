#include <tr1/memory>

#include "scene.h"

#include "loaders/texture_loader.h"
#include "loaders/q2bsp_loader.h"
#include "loaders/sprite_loader.h"
#include "loader.h"

#include "kazbase/logging/logging.h"

namespace kglt {
    
class WindowBase {
public:
    WindowBase():
        width_(0),
        height_(0),
        is_running_(true) {
        
        //Register the default resource loaders
        register_loader(LoaderType::ptr(new kglt::loaders::TextureLoaderType));
        register_loader(LoaderType::ptr(new kglt::loaders::Q2BSPLoaderType));
        register_loader(LoaderType::ptr(new kglt::loaders::SpriteLoaderType));
    }
    
    virtual ~WindowBase() {
        
    }
    
    Loader::ptr loader_for(const std::string& filename, const std::string& type_hint) {
        std::string final_file = find_file(filename);

        //See if we can find a loader that supports this type hint
        for(LoaderType::ptr loader_type: loaders_) {
            if(loader_type->has_hint(type_hint) && loader_type->supports(filename)) {
                return loader_type->loader_for(final_file);
            }
        }

        throw std::runtime_error("Unable to find a loader for: " + filename);
    }

    Loader::ptr loader_for(const std::string& filename) {
        std::string final_file = find_file(filename);

        for(LoaderType::ptr loader_type: loaders_) {
            if(loader_type->supports(final_file) && !loader_type->requires_hint()) {
                return loader_type->loader_for(final_file);
            }
        }

        throw std::runtime_error("Unable to find a loader for: " + filename);
    }    
    
    void register_loader(LoaderType::ptr loader_type);
    void add_search_path(const std::string& path) {
        resource_paths_.push_back(path);
    }

    virtual void set_title(const std::string& title) = 0;
    virtual void check_events() = 0;
    virtual void swap_buffers() = 0;
    
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    
    Scene& scene() { 
        if(!scene_) {
            scene_.reset(new Scene(this));
        }
        return *scene_; 
    }
    bool update();   

protected:
    void stop_running() { is_running_ = false; }
    
    void set_width(uint32_t width) { 
        width_ = width; 
    }
    
    void set_height(uint32_t height) {
        height_ = height; 
    }
    
private:
    std::tr1::shared_ptr<Scene> scene_;
    uint32_t width_;
    uint32_t height_;

    std::vector<std::string> resource_paths_;
    std::vector<LoaderType::ptr> loaders_;
    bool is_running_;
    
    std::string find_file(const std::string& filename) {
        //FIXME: Search the resource paths!
        return filename;
    }    
};

}
