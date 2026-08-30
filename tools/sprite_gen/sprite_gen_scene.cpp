#include "sprite_gen_scene.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <simulant/application.h>
#include <simulant/asset_manager.h>
#include <simulant/assets/prefab.h>
#include <simulant/color.h>
#include <simulant/compositor.h>
#include <simulant/logging.h>
#include <simulant/math/degrees.h>
#include <simulant/math/euler.h>
#include <simulant/math/quaternion.h>
#include <simulant/nodes/actor.h>
#include <simulant/nodes/armature.h>
#include <simulant/nodes/camera.h>
#include <simulant/nodes/light.h>
#include <simulant/nodes/prefab_instance.h>
#include <simulant/path.h>
#include <simulant/stage.h>
#include <simulant/renderers/renderer.h>
#include <simulant/viewport.h>
#include <simulant/window.h>

#include "dtex_writer.h"
#include "png_writer.h"
#include "tga_writer.h"

namespace sprite_gen {

namespace {

uint16_t next_pow2(uint32_t v) {
    uint32_t p = 1;
    while(p < v) {
        p <<= 1;
    }
    return uint16_t(std::min(p, uint32_t(MAX_SHEET_DIM)));
}

/* Runs texconv to write a compressed .dtex to `output_path`, feeding it a
 * temporary PNG of the sheet as input (texconv can't read pixels directly).
 * Requires `texconv_bin` (a name or full path) to be runnable via PATH. */
bool write_compressed_dtex(const std::string& texconv_bin,
                           const std::string& output_path, uint16_t width,
                           uint16_t height, const uint8_t* rgba) {
    namespace fs = std::filesystem;

    auto tmp_png =
        fs::temp_directory_path() /
        ("sprite_gen_" + std::to_string(std::rand()) + ".png");

    if(!write_png(tmp_png.string(), width, height, rgba)) {
        return false;
    }

    std::string cmd = "\"" + texconv_bin + "\"";
    cmd += " -i \"" + tmp_png.string() + "\"";
    cmd += " -o \"" + output_path + "\"";
    cmd += " -f ARGB4444";
    cmd += " -c";

    int rc = std::system(cmd.c_str());

    std::error_code ec;
    fs::remove(tmp_png, ec);

    if(rc != 0) {
        S_ERROR("texconv failed (exit {0}) - is it installed and on the "
                "PATH?",
                rc);
        return false;
    }

    return true;
}

} // namespace

SpriteGenScene::SpriteGenScene(smlt::Window* window, SpriteGenOptions opts):
    smlt::Scene(window), opts_(std::move(opts)) {}

void SpriteGenScene::fail() {
    *opts_.failed = true;
    app->stop_running();
}

void SpriteGenScene::on_load() {
    stage_ = create_child<smlt::Stage>();

    std::string ext = smlt::Path(opts_.input_path).ext();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if(ext == ".obj") {
        // .obj has no animation data, so this is always a single static
        // frame - load it as a plain mesh + Actor rather than a Prefab.
        auto mesh = assets->load_mesh(opts_.input_path);
        if(!mesh) {
            S_ERROR("Failed to load model '{0}' - check it's a valid .obj "
                    "file",
                    opts_.input_path);
            fail();
            return;
        }

        static_actor_ = stage_->create_child<smlt::Actor>(mesh);
        static_actor_->transform->set_rotation(smlt::Quaternion(smlt::Euler(
            opts_.rotation.x, opts_.rotation.y, opts_.rotation.z)));
        static_actor_->transform->set_scale_factor(
            smlt::Vec3(opts_.scale, opts_.scale, opts_.scale));

        if(!opts_.animation_name.empty()) {
            S_WARN("'.obj' models have no animations; --animation '{0}' "
                   "will be ignored, generating a single static frame",
                   opts_.animation_name);
        }
        frame_base_name_ = "frame";
        has_animation_ = false;
        frame_count_ = 1;
    } else {
        auto prefab = assets->load_prefab(opts_.input_path);
        if(!prefab) {
            S_ERROR("Failed to load model '{0}' - check it's a valid "
                    ".glb/.gltf/.obj file",
                    opts_.input_path);
            fail();
            return;
        }

        prefab_instance_ = stage_->create_child<smlt::PrefabInstance>(prefab);
        prefab_instance_->transform->set_rotation(smlt::Quaternion(smlt::Euler(
            opts_.rotation.x, opts_.rotation.y, opts_.rotation.z)));
        prefab_instance_->transform->set_scale_factor(
            smlt::Vec3(opts_.scale, opts_.scale, opts_.scale));

        anim_controller_ =
            prefab_instance_->find_mixin<smlt::AnimationController>();
        auto names = anim_controller_ ? anim_controller_->animation_names()
                                      : std::vector<std::string>();

        if(anim_controller_ && !names.empty()) {
            std::string anim_name = opts_.animation_name.empty()
                                        ? names.front()
                                        : opts_.animation_name;

            if(std::find(names.begin(), names.end(), anim_name) ==
               names.end()) {
                std::string available;
                for(std::size_t i = 0; i < names.size(); ++i) {
                    available += names[i];
                    if(i + 1 < names.size()) {
                        available += ", ";
                    }
                }
                S_ERROR("Animation '{0}' not found in '{1}'. Available: {2}",
                        anim_name, opts_.input_path, available);
                fail();
                return;
            }

            frame_base_name_ = anim_name;
            has_animation_ = true;

            float duration = anim_controller_->animation_duration(anim_name);
            frame_count_ = std::max(1, int(std::round(duration * opts_.fps)));

            anim_controller_->play(anim_name);
            anim_controller_->pause();
        } else {
            if(!opts_.animation_name.empty()) {
                S_WARN("Model has no animations; --animation '{0}' will be "
                       "ignored, generating a single static frame",
                       opts_.animation_name);
            }
            frame_base_name_ = "frame";
            has_animation_ = false;
            frame_count_ = 1;
        }
    }

    smlt::AABB bounds = compute_animated_bounds();
    setup_camera_and_layout(bounds);
    setup_lighting();

    sheet_pixels_.assign(std::size_t(sheet_w_) * sheet_h_ * 4, 0);
    capture_tmp_.assign(std::size_t(cell_w_) * cell_h_ * 4, 0);
    frame_infos_.reserve(std::size_t(frame_count_));

    S_INFO("Rendering '{0}' -> '{1}' ({2} frame(s), {3}x{4} sheet made of "
           "{5}x{6} cells)",
           opts_.input_path, opts_.output_path, frame_count_, sheet_w_,
           sheet_h_, cell_w_, cell_h_);

    pre_swap_connection_ =
        app->signal_pre_swap().connect([this]() { capture_frame(); });
}

smlt::AABB SpriteGenScene::compute_animated_bounds() {
    smlt::AABB bounds;
    bool found_any = false;

    auto accumulate_current_pose = [&]() {
        // Armatures re-pose into their own output mesh (and refresh its
        // AABB) every time the animation is seeked, so transformed_aabb()
        // tracks the current frame for both skinned and static geometry.
        // Search from stage_ rather than prefab_instance_ so this works
        // whether the model is a glb/gltf Prefab or a plain .obj Actor.
        auto renderers = stage_->find_descendents_by_types(
            {smlt::Actor::Meta::node_type, smlt::Armature::Meta::node_type});
        for(auto* node: renderers) {
            bounds.encapsulate(node->transformed_aabb());
            found_any = true;
        }
    };

    if(has_animation_) {
        for(int i = 0; i < frame_count_; ++i) {
            anim_controller_->seek(float(i) / opts_.fps);
            accumulate_current_pose();
        }
        anim_controller_->seek(0.0f);
    } else {
        accumulate_current_pose();
    }

    if(!found_any) {
        // No mesh-bearing descendants were found at all - fall back to a
        // small default box so the framing/layout math below doesn't
        // divide by zero.
        S_WARN("Model contains no visible meshes; framing with a "
               "default bounding box");
        return smlt::AABB(smlt::Vec3(), 1.0f);
    }

    return bounds;
}

void SpriteGenScene::setup_camera_and_layout(const smlt::AABB& bounds) {
    // Small margin so a rotating/animating model doesn't clip against the
    // exact edge of the frame due to floating point rounding, while still
    // keeping blank space to a minimum.
    const float margin = 1.02f;
    float half_w = std::max(0.001f, bounds.width() * 0.5f * margin);
    float half_h = std::max(0.001f, bounds.height() * 0.5f * margin);
    float half_d = std::max(0.001f, bounds.depth() * 0.5f);

    smlt::Vec3 center = bounds.center();
    float camera_distance = half_d + 10.0f;

    camera_ = stage_->create_child<smlt::Camera3D>();
    camera_->transform->set_position(
        smlt::Vec3(center.x, center.y, bounds.max().z + camera_distance));
    camera_->transform->look_at(center);

    // Grid layout: search every (cols, rows) combination that fits all the
    // frames - each reserving a FRAME_PADDING_PX-texel transparent border
    // on every side, so adjacent frames can't bleed into each other under
    // bilinear filtering - and pick the one that maximizes content
    // resolution within MAX_SHEET_DIM x MAX_SHEET_DIM (ties broken by
    // fewest wasted cells). Minimizing wasted cells alone isn't enough -
    // e.g. for 9 frames a 1x9 grid also wastes zero cells but crams each
    // frame into a sliver, while a 3x3 grid uses the same budget far more
    // effectively.
    float cell_aspect = half_w / half_h; // width:height, height normalised to 1

    uint16_t best_cols = 1;
    float best_scale = 0.0f;
    int best_waste = frame_count_;
    for(int cols = 1; cols <= frame_count_; ++cols) {
        int rows = (frame_count_ + cols - 1) / cols;
        int waste = cols * rows - frame_count_;

        float pad_w = float(cols) * 2.0f * FRAME_PADDING_PX;
        float pad_h = float(rows) * 2.0f * FRAME_PADDING_PX;
        if(pad_w >= float(MAX_SHEET_DIM) || pad_h >= float(MAX_SHEET_DIM)) {
            // Padding alone doesn't fit at this frame count - unusable.
            continue;
        }

        float scale =
            std::min((float(MAX_SHEET_DIM) - pad_w) / (float(cols) * cell_aspect),
                     (float(MAX_SHEET_DIM) - pad_h) / float(rows));
        if(scale > best_scale || (scale == best_scale && waste < best_waste)) {
            best_scale = scale;
            best_waste = waste;
            best_cols = uint16_t(cols);
        }
    }
    cols_ = best_cols;
    rows_ = uint16_t((frame_count_ + cols_ - 1) / cols_);

    float scale = best_scale;

    content_w_ = uint16_t(std::max(1.0f, std::floor(scale * cell_aspect)));
    content_h_ = uint16_t(std::max(1.0f, std::floor(scale)));

    cell_w_ = uint16_t(content_w_ + 2 * FRAME_PADDING_PX);
    cell_h_ = uint16_t(content_h_ + 2 * FRAME_PADDING_PX);

    // Inflate the camera frustum so the model only occupies content_w_ x
    // content_h_ pixels of the (larger) cell_w_ x cell_h_ viewport below,
    // leaving the reserved padding as untouched transparent border.
    float padded_half_w = half_w * (float(cell_w_) / float(content_w_));
    float padded_half_h = half_h * (float(cell_h_) / float(content_h_));
    camera_->set_orthographic_projection(-padded_half_w, padded_half_w,
                                         -padded_half_h, padded_half_h, 0.01f,
                                         camera_distance + half_d * 2.0f +
                                             10.0f);

    // Round the final sheet up to a power of two per axis (common
    // requirement for texture atlases, particularly on constrained
    // platforms). Any extra space beyond the packed grid stays transparent,
    // since the pixel buffer below is zero-initialized.
    sheet_w_ = next_pow2(uint32_t(cols_) * cell_w_);
    sheet_h_ = next_pow2(uint32_t(rows_) * cell_h_);

    auto layer = compositor->create_layer(stage_, camera_);
    layer->set_viewport(smlt::Viewport(
        smlt::Ratio(0.0f), smlt::Ratio(0.0f),
        smlt::Ratio(float(cell_w_) / float(window->width())),
        smlt::Ratio(float(cell_h_) / float(window->height()))));
    layer->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
    layer->viewport->set_color(smlt::Color(0.0f, 0.0f, 0.0f, 0.0f));
}

void SpriteGenScene::setup_lighting() {
    lighting->set_ambient_light(opts_.ambient_color);

    smlt::Light* light = nullptr;
    if(opts_.light_position) {
        auto point = stage_->create_child<smlt::PointLight>();
        point->transform->set_position(opts_.light_position.value());
        light = point;
    } else {
        auto directional = stage_->create_child<smlt::DirectionalLight>();
        directional->set_direction(
            opts_.light_direction.value_or(smlt::Vec3(-0.4f, -1.0f, -0.6f))
                .normalized());
        light = directional;
    }

    light->set_color(opts_.light_color);
    light->set_intensity(1.0f);
}

void SpriteGenScene::on_update(float dt) {
    smlt::Scene::on_update(dt);

    if(finished_) {
        return;
    }

    if(next_frame_ < frame_count_) {
        if(has_animation_) {
            anim_controller_->seek(float(next_frame_) / opts_.fps);
        }
        capture_pending_ = true;
    } else {
        finished_ = true;
        finalize();
    }
}

void SpriteGenScene::capture_frame() {
    if(!capture_pending_ || finished_) {
        return;
    }

    capture_pending_ = false;

    bool ok = window->renderer->read_pixels(0, 0, cell_w_, cell_h_,
                                            capture_tmp_.data());
    if(!ok) {
        S_ERROR("This renderer does not support framebuffer readback - "
                "cannot generate sprites");
        finished_ = true;
        fail();
        return;
    }

    int col = next_frame_ % cols_;
    int row = next_frame_ / cols_;
    uint16_t dst_x = uint16_t(col * cell_w_);
    uint16_t dst_y = uint16_t(row * cell_h_);

    // capture_tmp_ is bottom-to-top (OpenGL's readback convention), but
    // sheet_pixels_ is composed top-to-bottom (conventional image/atlas row
    // order) - flip rows while copying this cell in.
    for(uint16_t y = 0; y < cell_h_; ++y) {
        const uint8_t* src_row =
            capture_tmp_.data() +
            std::size_t(cell_h_ - 1 - y) * cell_w_ * 4;
        uint8_t* dst_row = sheet_pixels_.data() +
                           (std::size_t(dst_y + y) * sheet_w_ + dst_x) * 4;
        std::memcpy(dst_row, src_row, std::size_t(cell_w_) * 4);
    }

    // Recorded rect excludes the FRAME_PADDING_PX transparent border - the
    // atlas should describe each frame's actual tight content, not the
    // padded slot it's stored in.
    FrameInfo info;
    info.name = frame_base_name_ + "_" + std::to_string(next_frame_);
    info.x = uint16_t(dst_x + FRAME_PADDING_PX);
    info.y = uint16_t(dst_y + FRAME_PADDING_PX);
    info.w = content_w_;
    info.h = content_h_;
    frame_infos_.push_back(info);

    S_INFO("Captured frame {0}/{1}", next_frame_ + 1, frame_count_);

    next_frame_++;
}

void SpriteGenScene::finalize() {
    pre_swap_connection_.disconnect();

    std::string ext = smlt::Path(opts_.output_path).ext();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    bool ok;
    if(ext == ".tga") {
        ok = write_tga(opts_.output_path, sheet_w_, sheet_h_,
                       sheet_pixels_.data());
    } else {
        // .dtex is loaded with format_stored_upside_down() == false (see
        // dtex_loader.cpp), i.e. no flip happens on load, so row 0 in the
        // file must already be the bottom of the image. sheet_pixels_ is
        // composed top-to-bottom, and texconv doesn't flip its input either,
        // so flip here for both the direct writer and the PNG fed to
        // texconv under -C.
        std::vector<uint8_t> flipped(sheet_pixels_.size());
        const std::size_t row_bytes = std::size_t(sheet_w_) * 4;
        for(uint16_t y = 0; y < sheet_h_; ++y) {
            std::memcpy(flipped.data() + std::size_t(y) * row_bytes,
                       sheet_pixels_.data() +
                           std::size_t(sheet_h_ - 1 - y) * row_bytes,
                       row_bytes);
        }

        if(opts_.compress) {
            ok = write_compressed_dtex(opts_.texconv, opts_.output_path,
                                       sheet_w_, sheet_h_, flipped.data());
        } else {
            ok = write_dtex(opts_.output_path, sheet_w_, sheet_h_,
                            flipped.data());
        }
    }

    if(!ok) {
        fail();
        return;
    }
    S_INFO("Wrote sprite sheet: {0} ({1}x{2}, {3} frame(s))",
           opts_.output_path, sheet_w_, sheet_h_, frame_count_);

    if(!opts_.atlas_path.empty()) {
        auto image_filename = smlt::Path(opts_.output_path).name();
        if(!write_atlas_json(opts_.atlas_path, image_filename, sheet_w_,
                             sheet_h_, frame_infos_, frame_base_name_,
                             opts_.fps)) {
            fail();
            return;
        }
        S_INFO("Wrote atlas: {0}", opts_.atlas_path);
    }

    app->stop_running();
}

} // namespace sprite_gen
