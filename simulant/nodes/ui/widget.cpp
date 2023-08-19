#include <cmath>

#include "widget.h"
#include "ui_manager.h"
#include "../actor.h"
#include "../../stage.h"
#include "../../assets/material.h"
#include "../../window.h"
#include "../../application.h"

namespace smlt {
namespace ui {


Widget::Widget(Scene *owner, UIConfig defaults, StageNodeType type, std::shared_ptr<WidgetStyle> shared_style):
    ContainerNode(owner, type),
    owner_(owner),
    theme_(defaults),
    style_((shared_style) ? shared_style : std::make_shared<WidgetStyle>()) {

    if(!shared_style) {
        set_foreground_colour(defaults->foreground_colour_);
        set_background_colour(defaults->background_colour_);
        set_text_colour(defaults->text_colour_);
    }

    std::string family = (defaults->font_family_.empty()) ?
        get_app()->config->ui.font_family : defaults->font_family_;

    Px size = (defaults->font_size_ == Px(0)) ?
        Px(get_app()->config->ui.font_size) : Px(defaults->font_size_);

    auto font = stage->ui->load_or_get_font(family, size, FONT_WEIGHT_NORMAL, FONT_STYLE_NORMAL);
    assert(font);

    set_font(font);
}

Widget::~Widget() {
    if(focus_next_ && focus_next_->focus_previous_ == this) {
        focus_next_->focus_previous_ = nullptr;
    }

    if(focus_previous_ && focus_previous_->focus_next_ == this) {
        focus_previous_->focus_next_ = nullptr;
    }

    style_.reset();
}

bool Widget::on_init() {
    VertexSpecification spec = VertexSpecification::DEFAULT;

    /* We don't need normals or multiple texcoords */
    spec.normal_attribute = VERTEX_ATTRIBUTE_NONE;
    spec.texcoord1_attribute = VERTEX_ATTRIBUTE_NONE;

    mesh_ = stage->assets->new_mesh(spec);
    actor_ = stage->new_actor_with_mesh(mesh_);
    actor_->set_parent(this);

    /* Use the global materials until we can't anymore! */
    style_->materials_[WIDGET_LAYER_INDEX_BORDER] = stage->ui->global_border_material_;
    style_->materials_[WIDGET_LAYER_INDEX_BACKGROUND] = stage->ui->global_background_material_;
    style_->materials_[WIDGET_LAYER_INDEX_FOREGROUND] = stage->ui->global_foreground_material_;

    /* Now we must create the submeshes in the order we want them rendered */
    mesh_->new_submesh("border", style_->materials_[WIDGET_LAYER_INDEX_BORDER], MESH_ARRANGEMENT_TRIANGLE_STRIP);
    mesh_->new_submesh("background", style_->materials_[WIDGET_LAYER_INDEX_BACKGROUND], MESH_ARRANGEMENT_TRIANGLE_STRIP);
    mesh_->new_submesh("foreground", style_->materials_[WIDGET_LAYER_INDEX_FOREGROUND], MESH_ARRANGEMENT_TRIANGLE_STRIP);
    mesh_->new_submesh("text", font_->material(), MESH_ARRANGEMENT_TRIANGLE_STRIP);

    rebuild();

    initialized_ = true;
    return true;
}

void Widget::on_clean_up() {
    // Make sure we fire any outstanding events when the widget
    // is destroyed. If any buttons are held, then they should fire
    // released signals.
    force_release();
}

void Widget::set_font(const std::string& family, Rem size, FontWeight weight, FontStyle style) {
    Px final = owner_->config()->font_size_ * size;
    set_font(family, final, weight, style);
}

void Widget::set_font(const std::string& family, Px size, FontWeight weight, FontStyle style) {
    if(owner_) {
        set_font(owner_->load_or_get_font(family, size, weight, style));
    }
}

void Widget::set_font(FontPtr font) {
    if(!font) {
        S_WARN("Tried to set NULL font on a widget");
        return;
    }

    if(font_ && font && font_->id() == font->id()) {
        return;
    }

    font_ = font;

    on_size_changed();
}

void Widget::resize(Rem width, Px height) {
    assert(font_);

    Px w = Px(font_->size()) * width;
    resize(w, height);
}

void Widget::resize(Px width, Rem height) {
    assert(font_);
    Px h = Px(font_->size()) * height;
    resize(width, h);
}

void Widget::resize(Rem width, Rem height) {
    assert(font_);
    Px w = Px(float(font_->size()) * width.value);
    Px h = Px(float(font_->size()) * height.value);

    resize(w, h);
}

void Widget::resize(Px width, Px height) {
    if(requested_width_ == width && requested_height_ == height) {
        return;
    }

    if(width == Px(-1) && height > Px(-1)) {
        resize_mode_ = RESIZE_MODE_FIXED_HEIGHT;
    } else if(width == -1 && height == -1) {
        resize_mode_ = RESIZE_MODE_FIT_CONTENT;
    } else if(width > -1 && height == -1) {
        resize_mode_ = RESIZE_MODE_FIXED_WIDTH;
    } else {
        resize_mode_ = RESIZE_MODE_FIXED;
    }

    requested_width_ = width;
    requested_height_ = height;
    on_size_changed();
}

static bool is_visible_character(unicode::value_type ch) {
    return ch > 32;
}

void Widget::render_text() {
    struct Vertex {
        smlt::Vec3 xyz;
        smlt::Vec2 uv;
    };

    if(!font_ || text().empty()) {
        text_width_ = text_height_ = Px();
        return;
    }

    /* static to avoid reallocating each frame */
    static std::vector<std::pair<uint32_t, uint32_t>> line_ranges;
    static std::vector<Px> line_lengths;
    static std::vector<Vertex> vertices;

    /* Clear, but not shrink_to_fit */
    line_ranges.clear();
    line_lengths.clear();
    vertices.clear();

    /* We know how many vertices we'll need (roughly) */
    vertices.reserve(text().length() * 4);

    Px left_bound = 0;

    /* We don't have a right bound if the widget is supposed to fit the content
     * or if we have a fixed height, but unfixed width. Otherwise the right bound is
     * the requested width */
    Px right_bound = (
        resize_mode_ == RESIZE_MODE_FIT_CONTENT ||
        resize_mode_ == RESIZE_MODE_FIXED_HEIGHT
    ) ?
    std::numeric_limits<int>::max() :
    std::max(Px(0), (requested_width_ - (style_->padding_.left + style_->padding_.right)));

    Px left = left_bound;
    uint32_t line_start = 0;
    Px line_length = 0;
    uint32_t line_vertex_count = 0;

    /* Generate lines of text */
    auto text_ptr = &text()[0];
    auto text_length = text().length();

    for(uint32_t i = 0; i < text_length; ++i) {
        unicode::value_type ch = text_ptr[i];
        Px ch_width = font_->character_width(ch);
        Px ch_height = font_->character_height(ch);

        /* FIXME: This seems wrong.. if advance *should be* a float then we should probably
         * not do this cast here */
        Px ch_advance = (uint16_t) font_->character_advance(
            ch, (i < text_length - 1) ? text_ptr[i + 1] : ' '
        );

        auto right = left + ch_width;
        auto next_left = left + ch_advance;
        auto finalize_line = [&]() {
            if(line_length == 0) {
                return;
            }

            /* Ranges are (start_vertex_index, length_in_chars) */
            line_ranges.push_back(std::make_pair(line_start, line_vertex_count));
            line_lengths.push_back(line_length);
            line_start = vertices.size();
            left = left_bound;
            line_length = Px(0);
            line_vertex_count = 0;
        };

        bool break_line = ch == '\n';

        if(resize_mode() == RESIZE_MODE_FIXED || resize_mode() == RESIZE_MODE_FIXED_WIDTH) {

            // We always break if we're going beyond the bounds, that's true of
            // word or char wrapping. Word wrapping should detect this early unless
            // there's a super long word.
            if(right >= right_bound && ch_width < right_bound) {
                break_line = true;
            }

            if(wrap_mode() == WRAP_MODE_WORD && (ch == ' ' || ch == '\t') && i < text_length - 1) {
                // FIXME: is_whitespace()

                /* OK this is a whitespace character, and there are characters
                 * following it. So let's see how big the next word is */
                Px l = 0;

                for(std::size_t j = i + 1; j < text_length; ++j) {
                    auto wchr = text_ptr[j];
                    if((wchr == ' ' || wchr == '\t')) {
                        break;
                    }
                    // FIXME: Pass in the next char, not ' '
                    l += (uint16_t) font_->character_advance(wchr, ' ');
                }

                /* The next word will be beyond the bounds so let's break here */
                if(right + l >= right_bound) {
                    break_line = true;
                }
            }
        }

        if(break_line) {
            /* We reached the end of the line, so we finalize without
             * actually processing this character, then rewind one step */
            finalize_line();

            /* We replay the character if it's not a newline or whitespace
             * (e.g. we're wrapping width, not newline) */
            if(ch != '\n' && ch != ' ' && ch != '\t') i--;
            continue;
        }

        /* If the character is visible, then create some vertices for it */
        if(is_visible_character(ch)) {
            Vertex corners[4];

            // Characters are created with their top-line at 0, we then
            // properly manipulate the position when we process the lines later
            auto off = font_->character_offset(ch);

            auto top = -off.second;
            auto bottom = top - ch_height.value;

            top -= font_->ascent();
            bottom -= font_->ascent();

            corners[0].xyz = smlt::Vec3((left + off.first).value, bottom, 0);
            corners[1].xyz = smlt::Vec3((right + off.first).value, bottom, 0);
            corners[2].xyz = smlt::Vec3((right + off.first).value, top, 0);
            corners[3].xyz = smlt::Vec3((left + off.first).value, top, 0);

            auto min_max = font_->texture_coordinates_for_character(ch);
            corners[0].uv = smlt::Vec2(min_max.first.x, min_max.second.y);
            corners[1].uv = smlt::Vec2(min_max.second.x, min_max.second.y);
            corners[2].uv = smlt::Vec2(min_max.second.x, min_max.first.y);
            corners[3].uv = smlt::Vec2(min_max.first.x, min_max.first.y);

            vertices.push_back(corners[0]);
            vertices.push_back(corners[1]);
            vertices.push_back(corners[2]);
            vertices.push_back(corners[3]);

            line_vertex_count += 4;
        }

        /* Include visible characters and spaces in the length
         * of the line */
        if(is_visible_character(ch) || ch == ' ') {
            line_length += ch_advance;
        }

        if(i == text().length() - 1) {
            finalize_line();
        }

        left = next_left;
    }

    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();

    /* Now apply line heights */
    uint32_t j = 0;
    for(auto range: line_ranges) {
        if(!range.second) {
            // If there are no vertices on this line, then
            // ignore.
            continue;
        }

        uint16_t shift = (j * line_height().value);

        Vertex* ch = &vertices.at(range.first);
        uint16_t hw = line_lengths[j++].value / 2;
        for(auto i = 0u; i < range.second; i += 4) {
            Vertex* bl = ch;
            Vertex* br = ch + 1;
            Vertex* tr = ch + 2;
            Vertex* tl = ch + 3;

            // Shift the vertex downwards
            bl->xyz.y -= shift;
            br->xyz.y -= shift;
            tr->xyz.y -= shift;
            tl->xyz.y -= shift;

            // Center each line
            bl->xyz.x -= hw;
            br->xyz.x -= hw;
            tr->xyz.x -= hw;
            tl->xyz.x -= hw;

            min_x = std::min(bl->xyz.x, min_x);
            min_x = std::min(tl->xyz.x, min_x);
            max_x = std::max(br->xyz.x, max_x);
            max_x = std::max(tr->xyz.x, max_x);
            ch += 4;
        }
    }

    if(j == 0) {
        min_x = max_x = 0.0f;
    }

    auto sm = mesh_->find_submesh("text");
    assert(sm);

    /* Make sure the font material is up to date! */
    sm->set_material(font_->material());

    auto max_length = *std::max_element(line_lengths.begin(), line_lengths.end());

    text_width_ = max_length;
    text_height_ = line_height() * int(line_ranges.size());

    /* Shift the text depending on the difference in padding */
    auto diff = padding().left - padding().right;

    auto global_x_shift = diff.value; //(std::abs(min_x) - std::abs(max_x)) * 0.5f;
    auto global_y_shift = round(text_height_.value * 0.5f);

    auto vdata = mesh_->vertex_data.get();

    vdata->move_to_start();
    /* Allocate memory first */
    vdata->reserve(vertices.size());

    if(text_alignment() != TEXT_ALIGNMENT_CENTER) {
        auto cwidth = std::max(requested_width(), content_width()) - padding().left - padding().right;

        auto j = 0;
        for(auto range: line_ranges) {
            if(!range.second) {
                // If there are no vertices on this line, then
                // ignore.
                continue;
            }

            Vertex* ch = &vertices.at(range.first);

            auto ashift = std::ceil(-line_lengths[j++].value * 0.5f);
            ashift += std::ceil(cwidth.value * 0.5f);

            for(auto i = 0u; i < range.second; i++, ch++) {
                if(text_alignment() == TEXT_ALIGNMENT_LEFT) {
                    ch->xyz.x -= ashift;
                    ch->xyz.x += padding().left.value;
                } else {
                    ch->xyz.x += ashift;
                    ch->xyz.x -= padding().right.value;
                }
            }
        }
    }

    auto c = style_->text_colour_;
    c.set_alpha(style_->opacity_);

    auto idx = vdata->count();
    auto count = 0;
    for(std::size_t i = 0; i < vertices.size(); ++i) {
        Vertex* v = &vertices[i];

        /* Turn into a tri-strip, rather than a quad */
        if(i % 4 == 2) {
            v++;
        } else if(i % 4 == 3) {
            v--;
        }

        auto p = v->xyz + Vec3(global_x_shift, global_y_shift, 0);
        vdata->position(p);
        vdata->tex_coord0(v->uv);
        vdata->diffuse(c);
        vdata->move_next();

        /* Add this strip */
        if(count++ % 4 == 0) {
            sm->add_vertex_range(idx, 4);
            idx += 4;
        }
    }

    vdata->done();
}

void Widget::clear_mesh() {
    /* We don't actually clear the mesh, because destroying/creating submeshes is wasteful */

    mesh_->vertex_data->clear(/*release_memory=*/false);
    for(auto submesh: mesh_->each_submesh()) {
        if(submesh->type() == smlt::SUBMESH_TYPE_INDEXED) {
            submesh->index_data->clear();
        } else {
            submesh->remove_all_vertex_ranges();
        }
    }
}

SubMeshPtr Widget::new_rectangle(const std::string& name, WidgetBounds bounds, const smlt::Colour& colour, const Px& border_radius, const smlt::Vec2* uvs, float z_offset) {
    // Position so that the first rectangle is furthest from the
    // camera. Space for 10 layers (we only have 3 but whatevs.)

    auto sm = mesh_->find_submesh(name);
    assert(sm);

    sm->set_diffuse(colour);

    auto min = bounds.min;
    auto max = bounds.max;

    auto x_offset = 0.0f;
    auto y_offset = 0.0f;

    auto prev_count = mesh_->vertex_data->count();

    if(border_radius) {
        std::vector<Vec3> points;

        float half_height = (bounds.max.y - bounds.min.y).value / 2;
        int max_radius = std::min((bounds.max.x - bounds.min.x).value, (bounds.max.y - bounds.min.y).value) / 2;
        int radius = std::min((int) border_radius.value, max_radius);
        float fr = float(radius);

        /* First add left section */
        for(int i = 0; i < radius; ++i) {
            float r = (PI * 0.5f) * (float(i) / fr);
            float x = fr - (fr * std::cos(r));
            float y = fr - (fr * std::sin(r));

            points.push_back(Vec3(float(min.x.value) + x, half_height - y, z_offset));
            points.push_back(Vec3(float(min.x.value) + x, (-half_height) + y, z_offset));
        }

        /* Now add the central section */
        points.push_back(Vec3((min.x.value + radius), max.y.value, z_offset));
        points.push_back(Vec3((min.x.value + radius), min.y.value, z_offset));
        points.push_back(Vec3((max.x.value - radius), max.y.value, z_offset));
        points.push_back(Vec3((max.x.value - radius), min.y.value, z_offset));

        /* Finally the central section */
        for(int i = 0; i < radius; ++i) {
            float r = (PI * 0.5f) * (float(i) / fr);

            float x(fr * std::sin(r));
            float y(fr - (fr * std::cos(r)));

            int xstart = max.x.value - radius;

            points.push_back(Vec3(xstart + x, max.y.value - y, z_offset));
            points.push_back(Vec3(xstart + x, min.y.value + y, z_offset));
        }


        for(auto& p: points) {
            mesh_->vertex_data->move_to_end();
            mesh_->vertex_data->position(p);
            mesh_->vertex_data->diffuse(colour);
            mesh_->vertex_data->tex_coord0(0.0, 0.0f);
            mesh_->vertex_data->normal(0, 0, 1);
            mesh_->vertex_data->move_next();
        }

        sm->add_vertex_range(prev_count, points.size());
    } else {
        mesh_->vertex_data->move_to_end();
        mesh_->vertex_data->position(x_offset + min.x.value, y_offset + min.y.value, z_offset);
        mesh_->vertex_data->diffuse(colour);
        mesh_->vertex_data->tex_coord0((uvs) ? uvs[0].x : 0.0f, (uvs) ? uvs[0].y : 0.0f);
        mesh_->vertex_data->normal(0, 0, 1);
        mesh_->vertex_data->move_next();

        mesh_->vertex_data->position(x_offset + max.x.value, y_offset + min.y.value, z_offset);
        mesh_->vertex_data->diffuse(colour);
        mesh_->vertex_data->tex_coord0((uvs) ? uvs[1].x : 1.0f, (uvs) ? uvs[1].y : 0.0f);
        mesh_->vertex_data->normal(0, 0, 1);
        mesh_->vertex_data->move_next();

        mesh_->vertex_data->position(x_offset + min.x.value,  y_offset + max.y.value, z_offset);
        mesh_->vertex_data->diffuse(colour);
        mesh_->vertex_data->tex_coord0((uvs) ? uvs[2].x : 0.0f, (uvs) ? uvs[2].y : 1.0f);
        mesh_->vertex_data->normal(0, 0, 1);
        mesh_->vertex_data->move_next();
        mesh_->vertex_data->done();

        mesh_->vertex_data->position(x_offset + max.x.value,  y_offset + max.y.value, z_offset);
        mesh_->vertex_data->diffuse(colour);
        mesh_->vertex_data->tex_coord0((uvs) ? uvs[3].x : 1.0f, (uvs) ? uvs[3].y : 1.0f);
        mesh_->vertex_data->normal(0, 0, 1);
        mesh_->vertex_data->move_next();

        sm->add_vertex_range(prev_count, 4);
    }

    return sm;
}

void Widget::apply_image_rect(SubMeshPtr submesh, TexturePtr image, ImageRect& rect) {
    auto dim = image->dimensions();

    Vec2 min = Vec2(
        rect.bottom_left.x.value / dim.x,
        rect.bottom_left.y.value / dim.y
    );

    Vec2 max = Vec2(
        (rect.bottom_left.x.value + rect.size.x.value) / dim.x,
        (rect.bottom_left.y.value + rect.size.y.value) / dim.y
    );

    auto vertices = mesh_->vertex_data.get();

    if(!submesh->vertex_range_count()) {
        S_ERROR("Something went wrong with the generation of a widget. Could not apply image rect");
        return;
    }

    auto idx = submesh->vertex_ranges()[0].start;
    auto first_idx = idx;
    vertices->move_to(first_idx);
    vertices->tex_coord0(min.x, min.y);
    vertices->move_to(first_idx + 1);
    vertices->tex_coord0(max.x, min.y);
    vertices->move_to(first_idx + 3);
    vertices->tex_coord0(max.x, max.y);
    vertices->move_to(first_idx + 2);
    vertices->tex_coord0(min.x, max.y);
    vertices->done();
}

void Widget::render_border(const WidgetBounds& border_bounds) {
    auto colour = style_->border_colour_;
    float a = colour.af() * style_->opacity_;
    colour.set_alpha(a);
    /* FIXME! This should be 4 rectangles or a tri-strip */
    new_rectangle("border", border_bounds, colour, style_->border_radius_);
}

void Widget::render_background(const WidgetBounds& background_bounds) {
    auto colour = style_->background_colour_;
    colour.set_alpha(colour.af() * style_->opacity_);

    auto bg = new_rectangle("background", background_bounds, colour, style_->border_radius_);
    if(has_background_image()) {
        apply_image_rect(bg, style_->background_image_, style_->background_image_rect_);
    }
}

void Widget::render_foreground(const WidgetBounds& foreground_bounds) {
    auto colour = style_->foreground_colour_;
    colour.set_alpha(colour.af() * style_->opacity_);
    auto fg = new_rectangle("foreground", foreground_bounds, colour, style_->border_radius_);
    if(has_foreground_image()) {
        apply_image_rect(fg, style_->foreground_image_, style_->foreground_image_rect_);
    }
}

void Widget::rebuild() {
    // If we aren't initialized, don't do anything yet
    if(!is_initialized()) return;

    assert(mesh_);

    clear_mesh();
    _recalc_active_layers();

    prepare_build();

    // Sets the text width and height
    render_text();

    auto content_area = calculate_content_dimensions(
        text_width_,
        text_height_
    );

    content_width_ = content_area.width;
    content_height_ = content_area.height;

    auto background_bounds = calculate_background_size(content_area);
    auto foreground_bounds = calculate_foreground_size(content_area);

    auto border_bounds = background_bounds;
    border_bounds.min.x -= style_->border_width_;
    border_bounds.min.y -= style_->border_width_;
    border_bounds.max.x += style_->border_width_;
    border_bounds.max.y += style_->border_width_;

    if(border_active() && border_width() > 0) {
        render_border(border_bounds);
    }

    if(background_active() && background_bounds.has_non_zero_area()) {
        render_background(background_bounds);
    }

    if(foreground_active() && foreground_bounds.has_non_zero_area()) {
        render_foreground(foreground_bounds);
    }

    finalize_render();

    /* Apply anchoring */
    auto width = border_bounds.width().value;
    auto height = border_bounds.height().value;

    float xoff = -((anchor_point_.x * width) - (width / 2.0f));
    float yoff = -((anchor_point_.y * height) - (height / 2.0f));
    auto& vdata = mesh_->vertex_data;
    for(auto i = 0u; i < vdata->count(); ++i) {
        auto p = *vdata->position_at<smlt::Vec3>(i);
        p.x += xoff;
        p.y += yoff;
        vdata->move_to(i);
        vdata->position(p);
    }
    vdata->done();

    anchor_point_dirty_ = false;
    finalize_build();
}

Widget::WidgetBounds Widget::calculate_background_size(const UIDim& content_dimensions) const {
    Px box_width, box_height;

    // FIXME: Clipping + other modes
    if(resize_mode() == RESIZE_MODE_FIXED) {
        box_width = requested_width_;
        box_height = requested_height_;
    } else if(resize_mode_ == RESIZE_MODE_FIXED_WIDTH) {
        box_width = requested_width_;
        box_height = content_dimensions.height + style_->padding_.top + style_->padding_.bottom;
    } else if(resize_mode_ == RESIZE_MODE_FIXED_HEIGHT) {
        box_width = content_dimensions.width + style_->padding_.left + style_->padding_.right;
        box_height = requested_height_;
    } else {
        /* Fit content */
        box_width = content_dimensions.width + style_->padding_.left + style_->padding_.right;
        box_height = content_dimensions.height + style_->padding_.top + style_->padding_.bottom;
    }

    auto hw = Px((int16_t) std::ceil(float(box_width.value) * 0.5f));
    auto hh = Px((int16_t) std::ceil(float(box_height.value) * 0.5f));

    WidgetBounds bounds;
    bounds.min = UICoord(-hw, -hh);
    bounds.max = UICoord(hw, hh);
    return bounds;
}

Widget::WidgetBounds Widget::calculate_foreground_size(const UIDim& content_dimensions) const {
    return calculate_background_size(content_dimensions);
}

UIDim Widget::calculate_content_dimensions(Px text_width, Px text_height) {
    return UIDim(Px(text_width), Px(text_height));
}

void Widget::set_border_width(Px x) {
    if(style_->border_width_ == x) {
        return;
    }

    style_->border_width_ = x;
    rebuild();
}

Px Widget::border_width() const {
    return style_->border_width_;
}

void Widget::set_border_radius(Px x) {
    if(style_->border_radius_ == x) {
        return;
    }

    style_->border_radius_ = x;
    rebuild();
}

Px Widget::border_radius() const {
    return style_->border_radius_;
}

void Widget::set_border_colour(const Colour &colour) {
    if(style_->border_colour_ == PackedColour4444(colour)) {
        return;
    }

    style_->border_colour_ = colour;
    _recalc_active_layers();
    rebuild();
}

void Widget::set_padding(Px x) {
    set_padding(x, x, x, x);
}

void Widget::set_text(const unicode &text) {
    if(!pre_set_text(text)) {
        return;
    }

    if(text_ == text) {
        return;
    }

    text_ = text;
    on_size_changed();
}

void Widget::set_text_alignment(TextAlignment alignment) {
    style_->text_alignment_ = alignment;
}

TextAlignment Widget::text_alignment() const {
    return style_->text_alignment_;
}

void Widget::on_size_changed() {
    rebuild();
}

bool Widget::border_active() const {
    return (style_->active_layers_ & (1 << WIDGET_LAYER_INDEX_BORDER));
}

bool Widget::background_active() const {
    return (style_->active_layers_ & (1 << WIDGET_LAYER_INDEX_BACKGROUND));
}

bool Widget::foreground_active() const {
    return (style_->active_layers_ & (1 << WIDGET_LAYER_INDEX_FOREGROUND));
}

void Widget::_recalc_active_layers() {
    style_->active_layers_ = (
        (style_->border_colour_ != Colour::NONE) << WIDGET_LAYER_INDEX_BORDER |
        (style_->background_colour_ != Colour::NONE) << WIDGET_LAYER_INDEX_BACKGROUND |
        (style_->foreground_colour_ != Colour::NONE) << WIDGET_LAYER_INDEX_FOREGROUND |
        (style_->text_colour_ != Colour::NONE) << WIDGET_LAYER_INDEX_TEXT
    );
}

const AABB &Widget::aabb() const {
    if(actor_) {
        return actor_->aabb();
    } else {
        static AABB no_aabb;
        return no_aabb;
    }
}

void Widget::set_background_image(TexturePtr texture) {
    if(style_->background_image_ == texture) {
        return;
    }

    assert(owner_->global_background_material_);

    if(style_->materials_[WIDGET_LAYER_INDEX_BACKGROUND] == owner_->global_background_material_) {
        /* We need to switch to an independent material and can't use the global one anymore */
        style_->materials_[WIDGET_LAYER_INDEX_BACKGROUND] = owner_->clone_global_background_material();
        mesh_->find_submesh("background")->set_material(style_->materials_[WIDGET_LAYER_INDEX_BACKGROUND]);
    }

    style_->background_image_ = texture;
    style_->materials_[WIDGET_LAYER_INDEX_BACKGROUND]->set_diffuse_map(style_->background_image_);

    auto dim = texture->dimensions();
    // Triggers a rebuild
    set_background_image_source_rect(
        UICoord(),
        UICoord(Px(dim.x), Px(dim.y))
    );
}

void Widget::set_background_image_source_rect(const UICoord& bottom_left, const UICoord& size) {
    if(style_->background_image_rect_.bottom_left == bottom_left && style_->background_image_rect_.size == size) {
        // Nothing to do
        return;
    }

    style_->background_image_rect_.bottom_left = bottom_left;
    style_->background_image_rect_.size = size;
    rebuild();
}

void Widget::set_foreground_image(TexturePtr texture) {
    if(style_->foreground_image_ == texture) {
        return;
    }

    assert(owner_->global_foreground_material_);

    if(style_->materials_[WIDGET_LAYER_INDEX_FOREGROUND] == owner_->global_foreground_material_) {
        /* We need to switch to an independent material and can't use the global one anymore */
        style_->materials_[WIDGET_LAYER_INDEX_FOREGROUND] = owner_->clone_global_foreground_material();
        mesh_->find_submesh("foreground")->set_material(style_->materials_[WIDGET_LAYER_INDEX_FOREGROUND]);
    }

    style_->foreground_image_ = texture;
    style_->materials_[WIDGET_LAYER_INDEX_FOREGROUND]->set_diffuse_map(style_->foreground_image_);

    auto dim = texture->dimensions();

    // Triggers a rebuild
    set_foreground_image_source_rect(
        UICoord(),
        UICoord(Px(dim.x), Px(dim.y))
    );
}

void Widget::set_foreground_image_source_rect(const UICoord& bottom_left, const UICoord& size) {
    if(style_->foreground_image_rect_.bottom_left == bottom_left && style_->foreground_image_rect_.size == size) {
        // Nothing to do
        return;
    }

    style_->foreground_image_rect_.bottom_left = bottom_left;
    style_->foreground_image_rect_.size = size;
    rebuild();
}

void Widget::set_background_colour(const Colour& colour) {
    assert(style_);

    if(style_->background_colour_ == colour) {
        // Nothing to do
        return;
    }

    style_->background_colour_ = colour;

    _recalc_active_layers();
    rebuild();
}

void Widget::set_foreground_colour(const Colour& colour) {
    if(style_->foreground_colour_ == colour) {
        // Nothing to do
        return;
    }

    style_->foreground_colour_ = colour;

    _recalc_active_layers();
    rebuild();
}

void Widget::set_text_colour(const Colour &colour) {
    if(style_->text_colour_ == colour) {
        // Nothing to do
        return;
    }

    style_->text_colour_ = colour;
    _recalc_active_layers();
    rebuild();
}

Px Widget::requested_width() const {
    return requested_width_;
}

Px Widget::requested_height() const {
    return requested_height_;
}

Px Widget::content_width() const {
    return content_width_; // Content area
}

Px Widget::content_height() const {
    return content_height_;
}

Px Widget::outer_width() const {
    if(requested_width() == Px(-1)) {
        return content_width() + (style_->border_width_ * 2) + padding().left + padding().right;
    } else {
        return requested_width();
    }
}

Px Widget::outer_height() const {
    if(requested_height() == Px(-1)) {
        return content_height() + (style_->border_width_ * 2) + padding().top + padding().bottom;
    } else {
        return requested_height();
    }
}

bool Widget::set_resize_mode(ResizeMode resize_mode) {
    if(resize_mode == resize_mode_) {
        return false;
    }

    resize_mode_ = resize_mode;
    on_size_changed();
    return true;
}

ResizeMode Widget::resize_mode() const {
    return resize_mode_;
}

bool Widget::has_background_image() const {
    return bool(style_->background_image_);
}

bool Widget::has_foreground_image() const {
    return bool(style_->foreground_image_);
}

void Widget::set_padding(Px left, Px right, Px bottom, Px top) {
    if(left == style_->padding_.left &&
        right == style_->padding_.right &&
        bottom == style_->padding_.bottom &&
        top == style_->padding_.top) {

        return;
    }

    style_->padding_.left = left;
    style_->padding_.right = right;
    style_->padding_.bottom = bottom;
    style_->padding_.top = top;
    rebuild();
}

UInt4 Widget::padding() const {
    return style_->padding_;
}

bool Widget::is_pressed_by_finger(uint8_t finger_id) {
    return fingers_down_ & (1 << finger_id);
}

void Widget::force_release() {
    for(int i = 0; i < 16; ++i) {
        if(fingers_down_ & (1 << i)) {
            fingerup(i);
        }
    }
}

Vec2 Widget::anchor_point() const {
    return anchor_point_;
}

void Widget::set_opacity(RangeValue<0, 1> alpha) {
    if(style_->opacity_ != alpha) {
        style_->opacity_ = alpha;
        rebuild();
    }
}

Px Widget::line_height() const {
    return Px(font_->ascent() - font_->descent() + font_->line_gap());
}

void Widget::on_render_priority_changed(RenderPriority old_priority, RenderPriority new_priority) {
    assert(actor_);

    _S_UNUSED(old_priority);

    actor_->set_render_priority(new_priority);
}

void Widget::set_style(std::shared_ptr<WidgetStyle> style) {
    style_ = style;
    rebuild();
}

void Widget::set_anchor_point(RangeValue<0, 1> x, RangeValue<0, 1> y) {
    if(anchor_point_.x != (float) x || anchor_point_.y != (float) y) {
        anchor_point_ = smlt::Vec2(x, y);
        anchor_point_dirty_ = true;
    }
}

void Widget::fingerdown(uint8_t finger_id) {
    // If we added, and it was the first finger down

    bool first = !fingers_down_;

    if(!(fingers_down_ & (1 << finger_id))) {
        fingers_down_ |= (1 << finger_id);

        if(first) {
            // Emit the widget pressed signal
            signal_pressed_();
        }
    }
}

void Widget::fingerup(uint8_t finger_id) {
    if((fingers_down_ & (1 << finger_id))) {
        fingers_down_ &= ~(1 << finger_id);
        if(!fingers_down_) {
            // If we just released the last finger, emit a widget released signal
            signal_released_();
            signal_clicked_();
        }
    }
}

void Widget::fingerenter(uint8_t finger_id) {
    fingerdown(finger_id); // Same behaviour
}

void Widget::fingermove(uint8_t finger_id) {
    //FIXME: fire signal

}

void Widget::fingerleave(uint8_t finger_id) {
    // Same as fingerup, but we don't fire the click signal
    if((fingers_down_ & (1 << finger_id))) {
        fingers_down_ &= ~(1 << finger_id);
        if(!fingers_down_) {
            // If we just released the last finger, emit a widget released signal
            signal_released_();
        }
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

WidgetPtr Widget::next_in_focus_chain() const {
    return focus_next_;
}

WidgetPtr Widget::previous_in_focus_chain() const {
    return focus_previous_;
}

void Widget::click() {
    signal_clicked_();
}


}
}
