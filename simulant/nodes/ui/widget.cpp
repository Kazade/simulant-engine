#include <cmath>

#include "widget.h"
#include "ui_manager.h"
#include "../actor.h"
#include "../../stage.h"
#include "../../assets/material.h"

namespace smlt {
namespace ui {

Widget::Widget(UIManager *owner, UIConfig *defaults):
    TypedDestroyableObject<Widget, UIManager>(owner),
    ContainerNode(owner->stage(), STAGE_NODE_TYPE_OTHER),
    owner_(owner),
    pimpl_(new WidgetImpl()) {

}

Widget::~Widget() {
    if(pimpl_->focus_next_ && pimpl_->focus_next_->pimpl_->focus_previous_ == this) {
        pimpl_->focus_next_->pimpl_->focus_previous_ = nullptr;
    }

    if(pimpl_->focus_previous_ && pimpl_->focus_previous_->pimpl_->focus_next_ == this) {
        pimpl_->focus_previous_->pimpl_->focus_next_ = nullptr;
    }

    delete pimpl_;
}

bool Widget::init() {
    VertexSpecification spec = VertexSpecification::DEFAULT;

    /* We don't need normals or multiple texcoords */
    spec.normal_attribute = VERTEX_ATTRIBUTE_NONE;
    spec.texcoord1_attribute = VERTEX_ATTRIBUTE_NONE;

    mesh_ = stage->assets->new_mesh(spec);
    actor_ = stage->new_actor_with_mesh(mesh_);
    actor_->set_parent(this);

    material_ = stage->assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);
    if(!material_) {
        S_ERROR("[CRITICAL] Unable to load the material for widgets!");
        return false;
    }
    material_->set_blend_func(BLEND_ALPHA);
    material_->set_depth_test_enabled(false);
    material_->set_cull_mode(CULL_MODE_NONE);

    // Assign the default font as default
    auto font = stage->assets->default_font(DEFAULT_FONT_STYLE_BODY);
    if(!font) {
        S_ERROR("[CRITICAL] Unable to load the font for widgets!");
        return false;
    }

    set_font(font);

    /* Now we must create the submeshes in the order we want them rendered */
    mesh_->new_submesh_with_material("border", material_, MESH_ARRANGEMENT_QUADS);
    mesh_->new_submesh_with_material("background", material_, MESH_ARRANGEMENT_QUADS);
    mesh_->new_submesh_with_material("foreground", material_, MESH_ARRANGEMENT_QUADS);
    mesh_->new_submesh_with_material("text", font_->material_id(), MESH_ARRANGEMENT_QUADS);

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
    pimpl_->line_height_ = ::round(float(font_->size()) * 1.1f);

    on_size_changed();
}

void Widget::resize(int32_t width, int32_t height) {
    if(pimpl_->requested_width_ == width && pimpl_->requested_height_ == height) {
        return;
    }

    if(width == -1 && height > -1) {
        pimpl_->resize_mode_ = RESIZE_MODE_FIXED_HEIGHT;
    } else if(width == -1 && height == -1) {
        pimpl_->resize_mode_ = RESIZE_MODE_FIT_CONTENT;
    } else if(width > -1 && height == -1) {
        pimpl_->resize_mode_ = RESIZE_MODE_FIXED_WIDTH;
    } else {
        pimpl_->resize_mode_ = RESIZE_MODE_FIXED;
    }

    pimpl_->requested_width_ = width;
    pimpl_->requested_height_ = height;
    on_size_changed();
}

void Widget::render_text() {
    struct Vertex {
        smlt::Vec3 xyz;
        smlt::Vec2 uv;
    };

    if(text().empty()) {
        pimpl_->content_height_ = pimpl_->content_width_ = 0;
        return;
    }

    /* static to avoid reallocating each frame */
    static std::vector<std::pair<uint32_t, uint32_t>> line_ranges;
    static std::vector<float> line_lengths;
    static std::vector<Vertex> vertices;

    /* Clear, but not shrink_to_fit */
    line_ranges.clear();
    line_lengths.clear();
    vertices.clear();

    /* We know how many vertices we'll need (roughly) */
    vertices.reserve(text().length() * 4);

    float left_bound = 0;
    auto right_bound = pimpl_->requested_width_;
    float left = left_bound;
    uint32_t line_start = 0;
    float line_length = 0;
    uint32_t line_char_count = 0;


    /* Generate lines of text */
    for(uint32_t i = 0; i < text().length(); ++i) {
        unicode::value_type ch = text()[i];
        float ch_width = font_->character_width(ch);
        float ch_height = font_->character_height(ch);
        float ch_advance = (i == text().length() - 1) ? 0 : font_->character_advance(ch, text()[i + 1]);

        auto right = left + ch_width;
        auto next_left = left + ch_advance;
        auto finalize_line = [&]() {
            /* Ranges are (start_vertex_index, length_in_chars) */
            line_ranges.push_back(std::make_pair(line_start, line_char_count));
            line_lengths.push_back(line_length);
            line_start = vertices.size();
            left = left_bound;
            line_length = 0;
            line_char_count = 0;
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

            /* We replay the character if it's not a newline
             * (e.g. we're wrapping width, not newline) */
            if(ch != '\n') i--;
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
        line_char_count++;

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
        top += pimpl_->line_height_;
    }

    // Now we have to shift the entire thing up to vertically center!
    for(Vertex& v: vertices) {
        v.xyz.y += int(top / 2.0f);
    }

    auto sm = mesh_->find_submesh("text");
    assert(sm);

    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    auto vdata = mesh_->vertex_data.get();
    auto idata = sm->index_data.get();
    vdata->move_to_start();

    /* Allocate memory first */
    vdata->reserve(vertices.size());
    idata->reserve(vertices.size());

    auto idx = 0;
    for(auto& v: vertices) {
        min_x = std::min(min_x, v.xyz.x);
        max_x = std::max(max_x, v.xyz.x);
        min_y = std::min(min_y, v.xyz.y);
        max_y = std::max(max_y, v.xyz.y);

        vdata->position(v.xyz);
        vdata->tex_coord0(v.uv);
        vdata->diffuse(pimpl_->text_colour_);
        vdata->move_next();

        idata->index(idx++);
    }

    vdata->done();
    idata->done();

    pimpl_->content_width_ = max_x - min_x;
    pimpl_->content_height_ = max_y - min_y;
}

void Widget::clear_mesh() {
    /* We don't actually clear the mesh, because destroying/creating submeshes is wasteful */

    mesh_->vertex_data->clear(/*release_memory=*/false);
    for(auto submesh: mesh_->each_submesh()) {
        submesh->index_data->clear();
    }
}

SubMeshPtr Widget::new_rectangle(const std::string& name, WidgetBounds bounds, const smlt::Colour& colour) {
    // Position so that the first rectangle is furthest from the
    // camera. Space for 10 layers (we only have 3 but whatevs.)

    auto sm = mesh_->find_submesh(name);
    assert(sm);

    sm->set_diffuse(colour);

    auto offset = smlt::Vec3(bounds.width() / 2, bounds.height() / 2, 0) + smlt::Vec3(bounds.min.x, bounds.min.y, 0);
    auto width = bounds.width();
    auto height = bounds.height();
    auto x_offset = offset.x;
    auto y_offset = offset.y;

    /* We don't offset Z at all, submeshes are organised back to front
     * so that when rendering they should be correctly blended */
    auto z_offset = 0.0f;

    auto prev_count = mesh_->vertex_data->count();
    mesh_->vertex_data->move_to_end();

    mesh_->vertex_data->position(x_offset + (-width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    mesh_->vertex_data->diffuse(colour);
    mesh_->vertex_data->tex_coord0(0.0, 0.0f);
    mesh_->vertex_data->normal(0, 0, 1);
    mesh_->vertex_data->move_next();

    mesh_->vertex_data->position(x_offset + (width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    mesh_->vertex_data->diffuse(colour);
    mesh_->vertex_data->tex_coord0(1.0, 0.0f);
    mesh_->vertex_data->normal(0, 0, 1);
    mesh_->vertex_data->move_next();

    mesh_->vertex_data->position(x_offset + (width / 2.0f),  y_offset + (height / 2.0f), z_offset);
    mesh_->vertex_data->diffuse(colour);
    mesh_->vertex_data->tex_coord0(1.0, 1.0f);
    mesh_->vertex_data->normal(0, 0, 1);
    mesh_->vertex_data->move_next();

    mesh_->vertex_data->position(x_offset + (-width / 2.0f),  y_offset + (height / 2.0f), z_offset);
    mesh_->vertex_data->diffuse(colour);
    mesh_->vertex_data->tex_coord0(0.0, 1.0f);
    mesh_->vertex_data->normal(0, 0, 1);
    mesh_->vertex_data->move_next();
    mesh_->vertex_data->done();

    sm->index_data->index(prev_count + 0);
    sm->index_data->index(prev_count + 1);
    sm->index_data->index(prev_count + 2);
    sm->index_data->index(prev_count + 3);
    sm->index_data->done();

    return sm;
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

    assert(mesh_);

    clear_mesh();

    render_text();

    // FIXME: Clipping + other modes
    if(resize_mode() == RESIZE_MODE_FIXED) {
        pimpl_->content_width_ = pimpl_->requested_width_;
        pimpl_->content_height_ = pimpl_->requested_height_;
    } else if(pimpl_->resize_mode_ == RESIZE_MODE_FIXED_WIDTH) {
        pimpl_->content_height_ = std::max(pimpl_->requested_height_, pimpl_->content_height_);
        pimpl_->content_width_ = pimpl_->requested_width_;
    } else if(pimpl_->resize_mode_ == RESIZE_MODE_FIXED_HEIGHT) {
        pimpl_->content_width_ = std::max(pimpl_->requested_width_, pimpl_->content_width_);
        pimpl_->content_height_ = pimpl_->requested_height_;
    }

    auto background_bounds = calculate_background_size(pimpl_->content_width_, pimpl_->content_height_);
    auto foreground_bounds = calculate_foreground_size(pimpl_->content_width_, pimpl_->content_height_);

    auto border_bounds = background_bounds;
    border_bounds.min -= smlt::Vec2(pimpl_->border_width_, pimpl_->border_width_);
    border_bounds.max += smlt::Vec2(pimpl_->border_width_, pimpl_->border_width_);

    auto colour = pimpl_->border_colour_;
    colour.a *= pimpl_->opacity_;
    new_rectangle("border", border_bounds, colour);

    colour = pimpl_->background_colour_;
    colour.a *= pimpl_->opacity_;
    auto bg = new_rectangle("background", background_bounds, colour);
    if(has_background_image()) {
        bg->material()->pass(0)->set_diffuse_map(pimpl_->background_image_);
        apply_image_rect(bg, pimpl_->background_image_, pimpl_->background_image_rect_);
    }

    colour = pimpl_->foreground_colour_;
    colour.a *= pimpl_->opacity_;
    auto fg = new_rectangle("foreground", foreground_bounds, colour);
    if(has_foreground_image()) {
        fg->material()->pass(0)->set_diffuse_map(pimpl_->foreground_image_);
        apply_image_rect(fg, pimpl_->foreground_image_, pimpl_->foreground_image_rect_);
    }

    /* Apply anchoring */
    auto width = mesh_->aabb().width();
    auto height = mesh_->aabb().height();

    float xoff = -((pimpl_->anchor_point_.x * width) - (width / 2.0f));
    float yoff = -((pimpl_->anchor_point_.y * height) - (height / 2.0f));
    auto& vdata = mesh_->vertex_data;
    for(auto i = 0u; i < vdata->count(); ++i) {
        auto p = *vdata->position_at<smlt::Vec3>(i);
        p.x += xoff;
        p.y += yoff;
        vdata->move_to(i);
        vdata->position(p);
    }
    vdata->done();

    pimpl_->anchor_point_dirty_ = false;
}

Widget::WidgetBounds Widget::calculate_background_size(float content_width, float content_height) const {
    /* By default, we just return the content_width + padding */
    auto hw = content_width / 2.0f;
    auto hh = content_height / 2.0f;

    WidgetBounds bounds;
    bounds.min = smlt::Vec2(-(hw + pimpl_->padding_.left), -(hh + pimpl_->padding_.bottom));
    bounds.max = smlt::Vec2(hw + pimpl_->padding_.right, hh + pimpl_->padding_.top);
    return bounds;
}

Widget::WidgetBounds Widget::calculate_foreground_size(float content_width, float content_height) const {
    return calculate_background_size(content_width, content_height);
}

void Widget::set_border_width(float x) {
    if(pimpl_->border_width_ == x) {
        return;
    }

    pimpl_->border_width_ = x;
    rebuild();
}

void Widget::set_border_colour(const Colour &colour) {
    if(pimpl_->border_colour_ == colour) {
        return;
    }

    pimpl_->border_colour_ = colour;
    rebuild();
}

void Widget::set_text(const unicode &text) {
    if(pimpl_->text_ == text) {
        return;
    }

    pimpl_->text_ = text;
    on_size_changed();
}

void Widget::on_size_changed() {
    rebuild();
}

void Widget::set_property(const std::string &name, float value) {
    pimpl_->properties_[name] = value;
}

const AABB &Widget::aabb() const {
    return actor_->aabb();
}

void Widget::set_background_image(TexturePtr texture) {
    if(pimpl_->background_image_ == texture) {
        return;
    }

    pimpl_->background_image_ = texture;

    // Triggers a rebuild
    set_background_image_source_rect(
        Vec2(),
        texture->dimensions()
    );
}

void Widget::set_background_image_source_rect(const Vec2& bottom_left, const Vec2& size) {
    if(pimpl_->background_image_rect_.bottom_left == bottom_left && pimpl_->background_image_rect_.size == size) {
        // Nothing to do
        return;
    }

    pimpl_->background_image_rect_.bottom_left = bottom_left;
    pimpl_->background_image_rect_.size = size;
    rebuild();
}

void Widget::set_foreground_image(TexturePtr texture) {
    if(pimpl_->foreground_image_ == texture) {
        return;
    }

    pimpl_->foreground_image_ = texture;

    // Triggers a rebuild
    set_foreground_image_source_rect(
        Vec2(),
        texture->dimensions()
    );
}

void Widget::set_foreground_image_source_rect(const Vec2& bottom_left, const Vec2& size) {
    if(pimpl_->foreground_image_rect_.bottom_left == bottom_left && pimpl_->foreground_image_rect_.size == size) {
        // Nothing to do
        return;
    }

    pimpl_->foreground_image_rect_.bottom_left = bottom_left;
    pimpl_->foreground_image_rect_.size = size;
    rebuild();
}

void Widget::set_background_colour(const Colour& colour) {
    if(pimpl_->background_colour_ == colour) {
        // Nothing to do
        return;
    }

    pimpl_->background_colour_ = colour;
    rebuild();
}

void Widget::set_foreground_colour(const Colour& colour) {
    if(pimpl_->foreground_colour_ == colour) {
        // Nothing to do
        return;
    }

    pimpl_->foreground_colour_ = colour;
    rebuild();
}

void Widget::set_text_colour(const Colour &colour) {
    if(pimpl_->text_colour_ == colour) {
        // Nothing to do
        return;
    }

    pimpl_->text_colour_ = colour;
    rebuild();
}

float Widget::requested_width() const {
    return pimpl_->requested_width_;
}

float Widget::requested_height() const {
    return pimpl_->requested_height_;
}

float Widget::content_width() const {
    return pimpl_->content_width_; // Content area
}

float Widget::content_height() const {
    return pimpl_->content_height_;
}

float Widget::outer_width() const {
    return content_width() + (pimpl_->border_width_ * 2);
}

float Widget::outer_height() const {
    return content_height() + (pimpl_->border_width_ * 2);
}

void Widget::set_resize_mode(ResizeMode resize_mode) {
    pimpl_->resize_mode_ = resize_mode;
    on_size_changed();
}

ResizeMode Widget::resize_mode() const {
    return pimpl_->resize_mode_;
}

bool Widget::has_background_image() const {
    return bool(pimpl_->background_image_);
}

bool Widget::has_foreground_image() const {
    return bool(pimpl_->foreground_image_);
}

void Widget::set_padding(float left, float right, float bottom, float top) {
    pimpl_->padding_.left = left;
    pimpl_->padding_.right = right;
    pimpl_->padding_.bottom = bottom;
    pimpl_->padding_.top = top;
    rebuild();
}

bool Widget::is_pressed_by_finger(uint32_t finger_id) {
    return pimpl_->fingers_down_.find(finger_id) != pimpl_->fingers_down_.end();
}

void Widget::force_release() {
    auto fingers_down = pimpl_->fingers_down_; // Copy, fingerup will delete from fingers_down_
    for(auto& finger_id: fingers_down) {
        fingerup(finger_id);
    }
}

Vec2 Widget::anchor_point() const {
    return pimpl_->anchor_point_;
}

void Widget::set_opacity(RangeValue<0, 1> alpha) {
    if(pimpl_->opacity_ != alpha) {
        pimpl_->opacity_ = alpha;
        rebuild();
    }
}

void Widget::on_render_priority_changed(RenderPriority old_priority, RenderPriority new_priority) {
    assert(actor_);

    _S_UNUSED(old_priority);

    actor_->set_render_priority(new_priority);
}

void Widget::set_anchor_point(RangeValue<0, 1> x, RangeValue<0, 1> y) {
    if(pimpl_->anchor_point_.x != (float) x || pimpl_->anchor_point_.y != (float) y) {
        pimpl_->anchor_point_ = smlt::Vec2(x, y);
        pimpl_->anchor_point_dirty_ = true;
    }
}

void Widget::fingerdown(uint32_t finger_id) {
    // If we added, and it was the first finger down
    if(pimpl_->fingers_down_.insert(finger_id).second && pimpl_->fingers_down_.size() == 1) {
        // Emit the widget pressed signal
        signal_pressed_();
    }
}

void Widget::fingerup(uint32_t finger_id) {
    // If we just released the last finger, emit a widget released signal
    if(pimpl_->fingers_down_.erase(finger_id) && pimpl_->fingers_down_.empty()) {
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
    if(pimpl_->fingers_down_.erase(finger_id) && pimpl_->fingers_down_.empty()) {
        signal_released_();
    }
}

bool Widget::is_focused() const {
    return pimpl_->is_focused_;
}

void Widget::set_focus_previous(WidgetPtr previous_widget) {
    pimpl_->focus_previous_ = previous_widget;
    if(pimpl_->focus_previous_) {
        pimpl_->focus_previous_->pimpl_->focus_next_ = this;
    }
}

void Widget::set_focus_next(WidgetPtr next_widget) {
    pimpl_->focus_next_ = next_widget;
    if(pimpl_->focus_next_) {
        pimpl_->focus_next_->pimpl_->focus_previous_ = this;
    }
}

void Widget::focus() {
    auto focused = focused_in_chain_or_this();
    if(focused == this) {
        if(!pimpl_->is_focused_) {
            pimpl_->is_focused_ = true;
            signal_focused_();
        }
        return;
    } else {
        focused->blur();
        pimpl_->is_focused_ = true;
        signal_focused_();
    }
}

WidgetPtr Widget::focused_in_chain() {
    auto ret = focused_in_chain_or_this();

    // If we got back this element, but this element isn't focused
    // then return null
    if(ret == this && !pimpl_->is_focused_) {
        return nullptr;
    }

    return ret;
}

WidgetPtr Widget::focused_in_chain_or_this() {
    auto next = pimpl_->focus_next_;
    while(next && next != this) {
        if(next->is_focused()) {
            return next;
        }
        next = next->pimpl_->focus_next_;
    }

    auto previous = pimpl_->focus_previous_;
    while(previous && previous != this) {
        if(previous->is_focused()) {
            return previous;
        }
        previous = previous->pimpl_->focus_previous_;
    }

    return this;
}

void Widget::on_transformation_change_attempted() {
    // We do this is when a transformation change is attempted
    // (rather than if it happens)
    // because the anchor point may change and someone might
    // call move_to with the same position
    if(pimpl_->anchor_point_dirty_) {
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
        if(focused->pimpl_->focus_next_) {
            to_focus = focused->pimpl_->focus_next_;
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
        to_focus->pimpl_->is_focused_ = true;
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
        if(focused->pimpl_->focus_previous_) {
            to_focus = focused->pimpl_->focus_previous_;
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
        to_focus->pimpl_->is_focused_ = true;
        to_focus->signal_focused_();
    }
}

WidgetPtr Widget::first_in_focus_chain() {
    auto search = this;
    while(search->pimpl_->focus_previous_) {
        search = search->pimpl_->focus_previous_;
    }

    return search;
}

WidgetPtr Widget::last_in_focus_chain() {
    auto search = this;
    while(search->pimpl_->focus_next_) {
        search = search->pimpl_->focus_next_;
    }

    return search;
}

void Widget::blur() {
    pimpl_->is_focused_ = false;
    signal_blurred_();
}

void Widget::click() {
    signal_clicked_();
}


}
}
