#include <cmath>

#include "widget.h"
#include "ui_manager.h"
#include "../actor.h"
#include "../../stage.h"
#include "../../material.h"

namespace smlt {
namespace ui {

Widget::Widget(WidgetID id, UIManager *owner, UIConfig *defaults):
    ContainerNode(owner->stage()),
    generic::Identifiable<WidgetID>(id),
    owner_(owner) {

}

Widget::~Widget() {
    if(focus_next_ && focus_next_->focus_previous_ == this) {
        focus_next_->focus_previous_ = nullptr;
    }

    if(focus_previous_ && focus_previous_->focus_next_ == this) {
        focus_previous_->focus_next_ = nullptr;
    }
}

bool Widget::init() {
    VertexSpecification spec;
    spec.position_attribute = VERTEX_ATTRIBUTE_3F;
    spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    spec.diffuse_attribute = VERTEX_ATTRIBUTE_4F;

    mesh_ = stage->assets->new_mesh(spec);
    actor_ = stage->new_actor_with_mesh(mesh_);
    actor_->set_parent(this);

    material_ = stage->assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);
    material_->set_blend_func(BLEND_ALPHA);

    // Assign the default font as default
    set_font(stage->assets->default_font_id());
    rebuild();

    initialized_ = true;
    return true;
}

void Widget::clean_up() {
    // Make sure we fire any outstanding events when the widget
    // is destroyed. If any buttons are held, then they should fire
    // released signals.
    force_release();

    StageNode::clean_up();
}

void Widget::set_font(FontID font_id) {
    if(font_ && font_->id() == font_id) {
        return;
    }

    font_ = stage->assets->font(font_id);
    line_height_ = ::round(float(font_->size()) * 1.1);

    on_size_changed();
}

void Widget::resize(float width, float height) {
    if(requested_width_ == width && requested_height_ == height) {
        return;
    }

    requested_width_ = width;
    requested_height_ = height;
    on_size_changed();
}

void Widget::render_text() {
    struct Vertex {
        smlt::Vec3 xyz;
        smlt::Vec2 uv;
    };

    if(text().empty()) {
        content_height_ = content_width_ = 0;
        return;
    }

    // start and length, not start and end
    std::vector<std::pair<uint32_t, uint32_t>> line_ranges;
    std::vector<float> line_lengths;
    std::vector<Vertex> vertices;

    float left_bound = 0;
    auto right_bound = requested_width_;
    float left = left_bound;
    uint32_t line_start = 0;
    float line_length = 0;


    /* Generate lines of text */
    for(uint32_t i = 0; i < text().length(); ++i) {
        unicode::value_type ch = text()[i];
        float ch_width = font_->character_width(ch);
        float ch_height = font_->character_height(ch);
        float ch_advance = (i == text().length() - 1) ? 0 : font_->character_advance(ch, text()[i + 1]);

        auto right = left + ch_width;
        auto next_left = left + ch_advance;
        auto finalize_line = [&]() {
            line_ranges.push_back(std::make_pair(line_start, (i - line_start) + 1));
            line_lengths.push_back(line_length);
            line_start = vertices.size();
            left = left_bound;
            line_length = 0;
        };

        bool break_line = ch == '\n';

        if(resize_mode() == RESIZE_MODE_FIXED || resize_mode() == RESIZE_MODE_FIXED_WIDTH) {
            // FIXME: if(line_wrap)
            if(right >= right_bound) {
                break_line = true;
            }
        }

        if(break_line) {
            /* We reached the end of the line, so we finalize without
             * actually processing this character, then rewind one step */
            finalize_line();
            i--;
            continue;
        }

        Vertex corners[4];

        // Characters are created with their top-line at 0, we then
        // properly manipulate the position when we process the lines later
        auto off = font_->character_offset(ch);
        corners[0].xyz = smlt::Vec3(left + off.first, off.second - ch_height, 0);
        corners[1].xyz = smlt::Vec3(right + off.first, off.second - ch_height, 0);
        corners[2].xyz = smlt::Vec3(right + off.first, off.second, 0);
        corners[3].xyz = smlt::Vec3(left + off.first, off.second, 0);

        auto min_max = font_->texture_coordinates_for_character(ch);
        corners[0].uv = smlt::Vec2(min_max.first.x, min_max.second.y);
        corners[1].uv = smlt::Vec2(min_max.second.x, min_max.second.y);
        corners[2].uv = smlt::Vec2(min_max.second.x, min_max.first.y);
        corners[3].uv = smlt::Vec2(min_max.first.x, min_max.first.y);

        vertices.push_back(corners[0]);
        vertices.push_back(corners[1]);
        vertices.push_back(corners[2]);
        vertices.push_back(corners[3]);

        line_length += ch_advance;

        if(i == text().length() - 1) {
            finalize_line();
        }

        left = next_left;
    }

    /* Now apply line heights */
    float top = 0;
    uint32_t j = 0;
    for(auto range: line_ranges) {
        Vertex* ch = &vertices.at(range.first);
        float hw = line_lengths[j++] * 0.5f;
        for(auto i = 0u; i < range.second; ++i) {
            Vertex* bl = ch;
            Vertex* br = ch + 1;
            Vertex* tr = ch + 2;
            Vertex* tl = ch + 3;

            // Shift the vertex downwards
            bl->xyz.y -= top;
            br->xyz.y -= top;
            tr->xyz.y -= top;
            tl->xyz.y -= top;

            // Center each line
            bl->xyz.x -= hw;
            br->xyz.x -= hw;
            tr->xyz.x -= hw;
            tl->xyz.x -= hw;

            ch += 4;
        }

        // Increase for the next line
        top += line_height_;
    }

    // Now we have to shift the entire thing up to vertically center!
    for(Vertex& v: vertices) {
        v.xyz.y += top / 2.0f;
    }

    auto sm = mesh_->new_submesh_with_material("text", font_->material_id(), MESH_ARRANGEMENT_QUADS);

    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    auto vdata = mesh_->vertex_data.get();
    auto idata = sm->index_data.get();
    vdata->move_to_start();
    auto idx = 0;
    for(auto& v: vertices) {
        min_x = std::min(min_x, v.xyz.x);
        max_x = std::max(max_x, v.xyz.x);
        min_y = std::min(min_y, v.xyz.y);
        max_y = std::max(max_y, v.xyz.y);

        vdata->position(v.xyz);
        vdata->tex_coord0(v.uv);
        vdata->diffuse(text_colour_);
        vdata->move_next();

        idata->index(idx++);
    }

    vdata->done();
    idata->done();

    content_width_ = max_x - min_x;
    content_height_ = max_y - min_y;
}

void Widget::new_rectangle(const std::string& name, MaterialID mat_id, WidgetBounds bounds, const smlt::Colour& colour) {
    auto offset = smlt::Vec3(bounds.width() / 2, bounds.height() / 2, 0) + smlt::Vec3(bounds.min.x, bounds.min.y, 0);

    auto sm = mesh_->new_submesh_as_rectangle(name, mat_id, bounds.width(), bounds.height(), offset);
    sm->set_diffuse(colour);
}

void Widget::apply_image_rect(SubMeshPtr submesh, TexturePtr image, ImageRect& rect) {
    auto dim = image->dimensions();

    Vec2 min = Vec2(
        rect.bottom_left.x / dim.x,
        rect.bottom_left.y / dim.y
    );

    Vec2 max = Vec2(
        (rect.bottom_left.x + rect.size.x) / dim.x,
        (rect.bottom_left.y + rect.size.y) / dim.y
    );

    auto vertices = submesh->vertex_data.get();
    auto indices = submesh->index_data.get();

    auto first_idx = indices->at(0);
    vertices->move_to(first_idx);
    vertices->tex_coord0(min.x, min.y);
    vertices->move_to(first_idx + 1);
    vertices->tex_coord0(max.x, min.y);
    vertices->move_to(first_idx + 2);
    vertices->tex_coord0(max.x, max.y);
    vertices->move_to(first_idx + 3);
    vertices->tex_coord0(min.x, max.y);
    vertices->done();
}

void Widget::rebuild() {
    // If we aren't initialized, don't do anything yet
    if(!is_initialized()) return;

    mesh_->clear();

    render_text();

    // FIXME: Clipping + other modes
    if(resize_mode() == RESIZE_MODE_FIXED) {
        content_width_ = requested_width_;
        content_height_ = requested_height_;
    } else if(resize_mode_ == RESIZE_MODE_FIXED_WIDTH) {
        content_height_ = std::max(requested_height_, content_height_);
        content_width_ = requested_width_;
    } else if(resize_mode_ == RESIZE_MODE_FIXED_HEIGHT) {
        content_width_ = std::max(requested_width_, content_width_);
        content_height_ = requested_height_;
    }

    auto background_bounds = calculate_background_size(content_width_, content_height_);
    auto foreground_bounds = calculate_foreground_size(content_width_, content_height_);

    auto border_bounds = background_bounds;
    border_bounds.min -= smlt::Vec2(border_width_, border_width_);
    border_bounds.max += smlt::Vec2(border_width_, border_width_);

    auto colour = border_colour_;
    colour.a *= opacity_;
    new_rectangle("border", material_->id(), border_bounds, colour);

    colour = background_colour_;
    colour.a *= opacity_;
    new_rectangle("background", material_->id(), background_bounds, colour);
    if(has_background_image()) {
        auto sm = mesh_->submesh("background");
        sm->material_id().fetch()->set_diffuse_map(background_image_);
        apply_image_rect(sm, background_image_.fetch(), background_image_rect_);
    }

    colour = foreground_colour_;
    colour.a *= opacity_;
    new_rectangle("foreground", material_->id(), foreground_bounds, colour);
    if(has_foreground_image()) {
        auto sm = mesh_->submesh("foreground");
        sm->material_id().fetch()->set_diffuse_map(foreground_image_);
        apply_image_rect(sm, foreground_image_.fetch(), foreground_image_rect_);
    }

    /* Apply anchoring */
    auto width = mesh_->aabb().width();
    auto height = mesh_->aabb().height();

    float xoff = -((anchor_point_.x * width) - (width / 2.0));
    float yoff = -((anchor_point_.y * height) - (height / 2.0));
    auto& vdata = mesh_->vertex_data;
    for(auto i = 0u; i < vdata->count(); ++i) {
        auto p = vdata->position_at<smlt::Vec3>(i);
        p.x += xoff;
        p.y += yoff;
        vdata->move_to(i);
        vdata->position(p);
    }
    vdata->done();

    anchor_point_dirty_ = false;
}

Widget::WidgetBounds Widget::calculate_background_size(float content_width, float content_height) const {
    /* By default, we just return the content_width + padding */
    auto hw = content_width / 2.0;
    auto hh = content_height / 2.0;

    WidgetBounds bounds;
    bounds.min = smlt::Vec2(-(hw + padding_.left), -(hh + padding_.bottom));
    bounds.max = smlt::Vec2(hw + padding_.right, hh + padding_.top);
    return bounds;
}

Widget::WidgetBounds Widget::calculate_foreground_size(float content_width, float content_height) const {
    return calculate_background_size(content_width, content_height);
}

void Widget::set_border_width(float x) {
    if(border_width_ == x) {
        return;
    }

    border_width_ = x;
    rebuild();
}

void Widget::set_border_colour(const Colour &colour) {
    if(border_colour_ == colour) {
        return;
    }

    border_colour_ = colour;
    rebuild();
}

void Widget::set_width(float width) {
    if(requested_width_ == width) {
        return;
    }

    requested_width_ = width;
    on_size_changed();
}

void Widget::set_height(float height) {
    if(requested_height_ == height) {
        return;
    }

    requested_height_ = height;
    on_size_changed();
}

void Widget::set_text(const unicode &text) {
    if(text_ == text) {
        return;
    }

    text_ = text;
    on_size_changed();
}

void Widget::on_size_changed() {
    rebuild();
}

void Widget::set_property(const std::string &name, float value) {
    properties_[name] = value;
}

void Widget::destroy() {
    owner_->destroy_widget(id());
}

const AABB &Widget::aabb() const {
    return actor_->aabb();
}

void Widget::set_background_image(TextureID texture) {
    if(background_image_ == texture) {
        return;
    }

    background_image_ = texture;

    // Triggers a rebuild
    set_background_image_source_rect(
        Vec2(),
        texture.fetch()->dimensions()
    );
}

void Widget::set_background_image_source_rect(const Vec2& bottom_left, const Vec2& size) {
    if(background_image_rect_.bottom_left == bottom_left && background_image_rect_.size == size) {
        // Nothing to do
        return;
    }

    background_image_rect_.bottom_left = bottom_left;
    background_image_rect_.size = size;
    rebuild();
}

void Widget::set_foreground_image(TextureID texture) {
    if(foreground_image_ == texture) {
        return;
    }

    foreground_image_ = texture;

    // Triggers a rebuild
    set_foreground_image_source_rect(
        Vec2(),
        texture.fetch()->dimensions()
    );
}

void Widget::set_foreground_image_source_rect(const Vec2& bottom_left, const Vec2& size) {
    if(foreground_image_rect_.bottom_left == bottom_left && foreground_image_rect_.size == size) {
        // Nothing to do
        return;
    }

    foreground_image_rect_.bottom_left = bottom_left;
    foreground_image_rect_.size = size;
    rebuild();
}

void Widget::set_background_colour(const Colour& colour) {
    if(background_colour_ == colour) {
        // Nothing to do
        return;
    }

    background_colour_ = colour;
    rebuild();
}

void Widget::set_foreground_colour(const Colour& colour) {
    if(foreground_colour_ == colour) {
        // Nothing to do
        return;
    }

    foreground_colour_ = colour;
    rebuild();
}

void Widget::set_text_colour(const Colour &colour) {
    if(text_colour_ == colour) {
        // Nothing to do
        return;
    }

    text_colour_ = colour;
    rebuild();
}

void Widget::set_resize_mode(ResizeMode resize_mode) {
    resize_mode_ = resize_mode;
    on_size_changed();
}

void Widget::set_padding(float left, float right, float bottom, float top) {
    padding_.left = left;
    padding_.right = right;
    padding_.bottom = bottom;
    padding_.top = top;
    rebuild();
}

bool Widget::is_pressed_by_finger(uint32_t finger_id) {
    return fingers_down_.find(finger_id) != fingers_down_.end();
}

void Widget::force_release() {
    auto fingers_down = fingers_down_; // Copy, fingerup will delete from fingers_down_
    for(auto& finger_id: fingers_down) {
        fingerup(finger_id);
    }
}

Vec2 Widget::anchor_point() const {
    return anchor_point_;
}

void Widget::set_opacity(RangeValue<0, 1> alpha) {
    if(opacity_ != alpha) {
        opacity_ = alpha;
        rebuild();
    }
}

void Widget::set_anchor_point(RangeValue<0, 1> x, RangeValue<0, 1> y) {
    if(anchor_point_.x != (float) x || anchor_point_.y != (float) y) {
        anchor_point_ = smlt::Vec2(x, y);
        anchor_point_dirty_ = true;
    }
}

void Widget::fingerdown(uint32_t finger_id) {
    // If we added, and it was the first finger down
    if(fingers_down_.insert(finger_id).second && fingers_down_.size() == 1) {
        // Emit the widget pressed signal
        signal_pressed_();
    }
}

void Widget::fingerup(uint32_t finger_id) {
    // If we just released the last finger, emit a widget released signal
    if(fingers_down_.erase(finger_id) && fingers_down_.empty()) {
        signal_released_();
        signal_clicked_();
    }
}

void Widget::fingerenter(uint32_t finger_id) {
    fingerdown(finger_id); // Same behaviour
}

void Widget::fingermove(uint32_t finger_id) {
    //FIXME: fire signal
}

void Widget::fingerleave(uint32_t finger_id) {
    // Same as fingerup, but we don't fire the click signal
    if(fingers_down_.erase(finger_id) && fingers_down_.empty()) {
        signal_released_();
    }
}

bool Widget::is_focused() const {
    return is_focused_;
}

void Widget::set_focus_previous(WidgetPtr previous_widget) {
    focus_previous_ = previous_widget;
    if(focus_previous_) {
        focus_previous_->focus_next_ = this;
    }
}

void Widget::set_focus_next(WidgetPtr next_widget) {
    focus_next_ = next_widget;
    if(focus_next_) {
        focus_next_->focus_previous_ = this;
    }
}

void Widget::focus() {
    auto focused = focused_in_chain_or_this();
    if(focused == this) {
        if(!is_focused_) {
            is_focused_ = true;
            signal_focused_();
        }
        return;
    } else {
        focused->blur();
        is_focused_ = true;
        signal_focused_();
    }
}

WidgetPtr Widget::focused_in_chain() {
    auto ret = focused_in_chain_or_this();

    // If we got back this element, but this element isn't focused
    // then return null
    if(ret == this && !is_focused_) {
        return nullptr;
    }

    return ret;
}

WidgetPtr Widget::focused_in_chain_or_this() {
    auto next = focus_next_;
    while(next && next != this) {
        if(next->is_focused()) {
            return next;
        }
        next = next->focus_next_;
    }

    auto previous = focus_previous_;
    while(previous && previous != this) {
        if(previous->is_focused()) {
            return previous;
        }
        previous = previous->focus_previous_;
    }

    return this;
}

void Widget::on_transformation_change_attempted() {
    // We do this is when a transformation change is attempted
    // (rather than if it happens)
    // because the anchor point may change and someone might
    // call move_to with the same position
    if(anchor_point_dirty_) {
        // Reconstruct which will clear the dirty flag
        rebuild();
    }
}

void Widget::focus_next_in_chain(ChangeFocusBehaviour behaviour) {
    bool focus_none_if_none = (behaviour & FOCUS_NONE_IF_NONE_FOCUSED) == FOCUS_NONE_IF_NONE_FOCUSED;
    bool focus_this_if_none = (behaviour & FOCUS_THIS_IF_NONE_FOCUSED) == FOCUS_THIS_IF_NONE_FOCUSED;

    if(focus_none_if_none && focus_this_if_none) {
        throw std::logic_error(
            "You can only specify one 'none focused' behaviour"
        );
    }

    auto focused = focused_in_chain_or_this();
    auto to_focus = this;

    // If something is focused
    if(focused) {
        // Focus the next in the chain, otherwise focus the first in the chain
        if(focused->focus_next_) {
            to_focus = focused->focus_next_;
        } else {
            to_focus = first_in_focus_chain();
        }
    } else {
        // If nothing is focused, focus this if that's the desired behaviour
        // otherwise we don't focus anything
        if(focus_this_if_none) {
            to_focus = this;
        }
    }

    // Bail if we're already focusing the right thing
    if(focused && focused == to_focus) {
        return;
    }

    if(focused) {
        focused->blur();
    }

    if(to_focus) {
        to_focus->is_focused_ = true;
        to_focus->signal_focused_();
    }
}

void Widget::focus_previous_in_chain(ChangeFocusBehaviour behaviour) {
    bool focus_none_if_none = (behaviour & FOCUS_NONE_IF_NONE_FOCUSED) == FOCUS_NONE_IF_NONE_FOCUSED;
    bool focus_this_if_none = (behaviour & FOCUS_THIS_IF_NONE_FOCUSED) == FOCUS_THIS_IF_NONE_FOCUSED;

    if(focus_none_if_none && focus_this_if_none) {
        throw std::logic_error(
            "You can only specify one 'none focused' behaviour"
        );
    }

    auto focused = focused_in_chain_or_this();
    auto to_focus = this;

    // If something is focused
    if(focused) {
        // Focus the previous in the chain, otherwise focus the last in the chain
        if(focused->focus_previous_) {
            to_focus = focused->focus_previous_;
        } else {
            to_focus = last_in_focus_chain();
        }
    } else {
        // If nothing is focused, focus this if that's the desired behaviour
        // otherwise we don't focus anything
        if(focus_this_if_none) {
            to_focus = this;
        }
    }

    // Bail if we're already focusing the right thing
    if(focused && focused == to_focus) {
        return;
    }

    if(focused) {
        focused->blur();
    }

    if(to_focus) {
        to_focus->is_focused_ = true;
        to_focus->signal_focused_();
    }
}

WidgetPtr Widget::first_in_focus_chain() {
    auto search = this;
    while(search->focus_previous_) {
        search = search->focus_previous_;
    }

    return search;
}

WidgetPtr Widget::last_in_focus_chain() {
    auto search = this;
    while(search->focus_next_) {
        search = search->focus_next_;
    }

    return search;
}

void Widget::blur() {
    is_focused_ = false;
    signal_blurred_();
}

void Widget::click() {
    signal_clicked_();
}


}
}
