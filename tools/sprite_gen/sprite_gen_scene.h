#pragma once

#include <cstdint>
#include <vector>

#include <simulant/math/aabb.h>
#include <simulant/scenes/scene.h>
#include <simulant/signals/signal.h>

#include "atlas_writer.h"
#include "sprite_gen_options.h"

namespace smlt {
class PrefabInstance;
class AnimationController;
class Camera3D;
class Light;
} // namespace smlt

namespace sprite_gen {

/* The maximum width/height of the generated sprite sheet texture, and
 * therefore also the (square, fixed) size of the window - the window has
 * to be at least this big so that a single full-quality cell can always
 * be read back from the real framebuffer. Also used as the cap when
 * rounding the final sheet dimensions up to a power of two. */
constexpr uint16_t MAX_SHEET_DIM = 1024;

/* Minimum number of fully-transparent texels reserved around every side of
 * each frame's actual content, so that bilinear-filtered sampling near a
 * frame's edge can't pick up color from an adjacent frame in the sheet. */
constexpr uint16_t FRAME_PADDING_PX = 2;

class SpriteGenScene: public smlt::Scene {
public:
    SpriteGenScene(smlt::Window* window, SpriteGenOptions opts);

    void on_load() override;
    void on_update(float dt) override;

private:
    void fail();

    smlt::AABB compute_animated_bounds();
    void setup_camera_and_layout(const smlt::AABB& bounds);
    void setup_lighting();
    void capture_frame();
    void finalize();

    SpriteGenOptions opts_;

    smlt::Stage* stage_ = nullptr;
    smlt::PrefabInstance* prefab_instance_ = nullptr;
    smlt::AnimationController* anim_controller_ = nullptr;
    smlt::Camera3D* camera_ = nullptr;

    bool has_animation_ = false;
    std::string frame_base_name_;
    int frame_count_ = 1;
    int next_frame_ = 0;
    bool capture_pending_ = false;
    bool finished_ = false;

    uint16_t cols_ = 1;
    uint16_t rows_ = 1;
    uint16_t cell_w_ = 0;    // slot size: content + 2*FRAME_PADDING_PX
    uint16_t cell_h_ = 0;
    uint16_t content_w_ = 0; // tight size of the rendered frame within its slot
    uint16_t content_h_ = 0;
    uint16_t sheet_w_ = 0;   // final sheet size, rounded up to a power of two
    uint16_t sheet_h_ = 0;

    std::vector<uint8_t> sheet_pixels_; // RGBA, top-to-bottom, row-major
    std::vector<uint8_t> capture_tmp_;  // scratch buffer for one cell's readback
    std::vector<FrameInfo> frame_infos_;

    smlt::sig::Connection pre_swap_connection_;
};

} // namespace sprite_gen
