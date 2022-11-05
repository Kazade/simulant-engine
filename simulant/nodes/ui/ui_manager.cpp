
/*
 * Widget Input Explanation
 *
 * Widget input is hard. Widgets can be in any stage, be transformed in any way and can be rendered with any camera to any viewport. That
 * makes processing input events tricky as the coordinates for touches/clicks are in window coordinates and we won't know how to transform them
 * until we process the render queue.
 *
 * So, we tackle this by making the UIManager an event listener. When we receive events we care about, we store them in a queue.
 *
 * The queue is processed each time the owning stage is rendered, but the queue is not cleared! This is because you can render the same stage
 * multiple times. The queue is cleared at the end of each frame.
 */

#include "ui_manager.h"
#include "button.h"
#include "label.h"
#include "progress_bar.h"
#include "image.h"
#include "frame.h"
#include "keyboard.h"

#include "../../stage.h"
#include "../camera.h"
#include "../../window.h"
#include "../stage_node_manager.h"
#include "../../viewport.h"
#include "../../application.h"
#include "../../vfs.h"

namespace smlt {
namespace ui {

using namespace std::placeholders;

UIManager::UIManager(Stage *stage, StageNodePool *pool, UIConfig config):
    stage_(stage),
    config_(config) {    

    manager_.reset(new WidgetManager(pool));

    auto window = get_app()->window.get();
    window->register_event_listener(this);

    /* Each time the stage is rendered with a camera and viewport, we need to process any queued events
     * so that (for example) we can interact with the same widget rendered to different viewports */
    pre_render_connection_ = stage_->signal_stage_pre_render().connect([this](CameraID cam_id, Viewport viewport) {
        this->process_event_queue(cam_id.fetch(), viewport);
    });

    /* We clear queued events at the end of each frame */
    frame_finished_connection_ = get_app()->signal_frame_finished().connect([this]() {
        this->clear_event_queue();
    });

    auto new_material = [&]() -> smlt::MaterialPtr {
        auto material = stage_->asset_manager_->new_material_from_file(
            Material::BuiltIns::TEXTURE_ONLY
        );

        if(!material) {
            S_ERROR("[CRITICAL] Unable to load the material for widgets!");
            return smlt::MaterialPtr();
        }

        material->set_blend_func(BLEND_ALPHA);
        material->set_depth_test_enabled(false);
        material->set_cull_mode(CULL_MODE_NONE);
        return material;
    };

    global_background_material_ = new_material();
    global_foreground_material_ = new_material();
    global_border_material_ = new_material();
}

UIManager::~UIManager() {
    manager_->clear();
    manager_.reset();

    pre_render_connection_.disconnect();
    frame_finished_connection_.disconnect();

    auto app = get_app();
    if(app) {
        auto window = app->window.get();

        if(window) {
            window->unregister_event_listener(this);
        }
    }
}

Keyboard* UIManager::new_widget_as_keyboard(const KeyboardMode& mode, const unicode &initial_text) {
    auto keyboard = manager_->make_as<Keyboard>(this, &config_, stage_, mode, initial_text);
    stage_->add_child(keyboard);
    return keyboard;
}

Frame* UIManager::new_widget_as_frame(const unicode& title, const Px& width, const Px& height) {
    auto frame = manager_->make_as<Frame>(this, &config_, stage_);
    frame->set_text(title);
    frame->resize(width, height);
    stage_->add_child(frame);

    return frame;
}

Button* UIManager::new_widget_as_button(const unicode &text, Px width, Px height, std::shared_ptr<WidgetStyle> shared_style) {
    auto button = manager_->make_as<Button>(this, &config_, stage_, shared_style);
    button->set_text(text);
    button->resize(width, height);
    stage_->add_child(button);

    return button;
}

Label* UIManager::new_widget_as_label(const unicode &text, Px width, Px height) {
    auto label = (Label*) &(*manager_->make_as<Label>(this, &config_, stage_));
    label->set_text(text);
    label->resize(width, height);

    stage_->add_child(label);

    return label;
}

Image* UIManager::new_widget_as_image(const TexturePtr& texture) {
    auto image = (Image*) &(*manager_->make_as<Image>(this, &config_, stage_));
    image->set_texture(texture);

    stage_->add_child(image);

    return image;
}

Widget* UIManager::widget(WidgetID widget_id) {
    return manager_->get(widget_id);
}

ProgressBar* UIManager::new_widget_as_progress_bar(float min, float max, float value) {
    auto pg = (ProgressBar*) &(*manager_->make_as<ProgressBar>(this, &config_, stage_));

    pg->set_range(min, max);
    pg->set_value(value);

    stage_->add_child(pg);

    return pg;
}

void UIManager::destroy_widget(WidgetID widget_id) {
    if(!widget_id) {
        return;
    }

    auto w = widget(widget_id);
    if(!w) {
        return;
    }

    // Release any presses on the widget
    w->force_release();

    // Queue for destruction
    manager_->destroy(widget_id);
}

void UIManager::destroy_object(Widget* object) {
    // Release any presses on the widget
    object->force_release();

    // Queue for destruction
    manager_->destroy(object->id());
}

void UIManager::destroy_object_immediately(Widget* object) {
    object->force_release();
    manager_->destroy_immediately(object->id());
}

void UIManager::on_touch_begin(const TouchEvent &evt) {
    queue_event(evt);
}

void UIManager::on_touch_end(const TouchEvent &evt) {
    queue_event(evt);
}

void UIManager::on_touch_move(const TouchEvent &evt) {
    queue_event(evt);
}

void UIManager::queue_event(const TouchEvent& e) {
    UIEvent evt(e);
    queued_events_.push_back(evt);
}

void UIManager::process_event_queue(const Camera* camera, const Viewport &viewport) const {
    if(queued_events_.empty()) {
        return;
    }

    auto queued_events = queued_events_; // Copy the queue

    for(auto& evt: queued_events) {
        switch(evt.type) {
            case UI_EVENT_TYPE_TOUCH: {
                auto widget = find_widget_at_window_coordinate(camera, viewport, Vec2(evt.touch.coord.x, evt.touch.coord.y));
                if(widget) {
                    if(evt.touch.type == TOUCH_EVENT_TYPE_FINGER_DOWN) {
                        widget->fingerdown(evt.touch.touch_id);
                    } else if(evt.touch.type == TOUCH_EVENT_TYPE_FINGER_UP) {
                        widget->fingerup(evt.touch.touch_id);
                    } else if(evt.touch.type == TOUCH_EVENT_TYPE_FINGER_MOVE) {
                        // If we just moved over the widget, and we weren't already on it
                        // then trigger a fingerenter
                        if(!widget->is_pressed_by_finger(evt.touch.touch_id)) {
                            widget->fingerenter(evt.touch.touch_id);
                        }

                        // notify that there was a movement on the widget
                        widget->fingermove(evt.touch.touch_id);
                    }
                }

                if(evt.touch.type == TOUCH_EVENT_TYPE_FINGER_MOVE || evt.touch.type == TOUCH_EVENT_TYPE_FINGER_UP) {
                    // Go through all the widgets, if one is being pressed and it's different
                    // than the one above, then trigger a fingerleave event
                    for(auto iter: *manager_) {
                        if(iter->is_pressed_by_finger(evt.touch.touch_id) && iter != widget) {
                            iter->fingerleave(evt.touch.touch_id);
                        }
                    }
                }

            } break;
        default:
            break;
        }
    }
}

void UIManager::clear_event_queue() {
    queued_events_.clear();
}

WidgetPtr UIManager::find_widget_at_window_coordinate(const Camera *camera, const Viewport &viewport, const Vec2 &window_coord) const {
    WidgetPtr result = nullptr;

    auto window = get_app()->window.get();

    for(auto widget: *manager_) {
        auto aabb = widget->transformed_aabb();
        std::vector<Vec3> ss_points;

        for(auto& corner: aabb.corners()) {
            ss_points.push_back(camera->project_point(*window, viewport, corner).value());
        }

        AABB ss_aabb(&ss_points[0], ss_points.size());

        // FIXME: Return the nearest if overlapping!
        if(ss_aabb.contains_point(Vec3(window_coord, 0.5))) {
            result = widget;
        }
    };

    return result;
}

FontPtr UIManager::load_or_get_font(const std::string& family, const Px& size, const FontWeight& weight, const FontStyle &style) {
    return _load_or_get_font(
        get_app()->vfs, stage_->assets, get_app()->shared_assets.get(),
        family, size, weight, style
    );
}

FontPtr UIManager::_load_or_get_font(
        VirtualFileSystem* vfs, AssetManager* assets, AssetManager* shared_assets,
        const std::string &familyc, const Px &sizec, const FontWeight& weight, const FontStyle& style) {

    static LRUCache<std::string, smlt::Path> location_cache;
    location_cache.set_max_size(8);

    /* Apply defaults if that's what was asked */

    std::string family = familyc;
    if(family == DEFAULT_FONT_FAMILY) {
        family = get_app()->config->ui.font_family;
    }

    Px size = sizec;
    if(size == DEFAULT_FONT_SIZE) {
        size = get_app()->config->ui.font_size;
    }

    const std::string px_as_string = smlt::to_string(size.value);
    const std::string weight_string = font_weight_name(weight);
    const std::string style_string = (style == FONT_STYLE_NORMAL) ? "" : font_style_name(style);

    std::string identifier = _F("{0}-{1}-{2}-{3}").format(familyc, px_as_string, weight_string, style_string);

    /* We search for standard variations of the filename depending on the family,
     * weight, style, and size. We look for the following (example) variations:
     *
     * - Kanit-Regular.ttf
     * - Kanit-RegularItalic.ttf
     * - Kanit-BlackItalic.ttf
     * - Kanit-BlackItalic-18.fnt
     */

    std::string potentials [] = {        
        family + "-" + weight_string + style_string + ".ttf",
        family + "-" + weight_string + style_string + "-" + px_as_string + ".fnt",
    };

    std::string alias = Font::generate_name(family, size.value, weight, style);

    /* See if the font is already loaded, first look at the stage
     * level, but fallback to the window level (in case it was pre-loaded
     * globally) */
    FontPtr fnt;
    if(assets) {
        fnt = assets->find_font(alias);
    }

    if(!fnt && shared_assets) {
        fnt = shared_assets->find_font(alias);
    }

    /* We already loaded it, all is well! */
    if(fnt) {
        return fnt;
    }

    auto loc = location_cache.get(identifier);
    if(!loc) {
        std::map<smlt::Path, bool> pushed;

        /* Extend the search path with the specified font directories */
        for(auto& font_dir: get_app()->config->ui.font_directories) {
            pushed[font_dir] = vfs->add_search_path(font_dir);
        }

        for(auto& filename: potentials) {
            loc = vfs->locate_file(filename, true, /*fail_silently=*/true);
            if(loc) {
                break;
            }

            /* Try a font directory prefix */
            loc = vfs->locate_file(kfs::path::join("fonts", filename), true, /*fail_silently=*/true);
            if(loc) {
                break;
            }

            /* Finally try a family name dir within fonts */
            loc = vfs->locate_file(
                kfs::path::join(kfs::path::join("fonts", family), filename),
                true, /*fail_silently=*/true
            );

            if(loc) {
                location_cache.insert(identifier, loc.value());
                break;
            }
        }

        /* Remove appended font dirs */
        for(auto& p: pushed) {
            if(p.second) {
                vfs->remove_search_path(p.first);
            }
        }
    } else {
        S_DEBUG("Found cached font location");
    }

    if(!loc) {
        S_WARN("Unable to locate font file with family {0} and size {1}", family, size.value);
        return FontPtr(); /* Fail */
    }

    FontFlags flags;
    flags.size = size.value;
    flags.weight = weight;

    S_DEBUG("Loaded font with family {0} and size {1} from {2}", family, size.value, loc.value().str());
    fnt = assets->new_font_from_file(loc.value(), flags);
    fnt->set_name(alias);
    return fnt;
}

MaterialPtr UIManager::clone_global_background_material() {
    assert(global_background_material_);
    return stage_->asset_manager_->clone_material(global_background_material_);
}

MaterialPtr UIManager::clone_global_foreground_material() {
    assert(global_foreground_material_);
    return stage_->asset_manager_->clone_material(global_foreground_material_);
}

}
}
