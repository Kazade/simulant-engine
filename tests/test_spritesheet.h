#pragma once

#include <fstream>
#include <vector>

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

/* Writes a minimal 18-byte-header uncompressed TGA (matches the format
 * written by tools/sprite_gen/tga_writer.cpp) so the spritesheet loader has
 * a real texture file to resolve. */
void write_spritesheet_test_texture(const std::string& path, uint16_t w,
                                    uint16_t h) {
    uint8_t header[18] = {};
    header[2] = 2; // uncompressed truecolor
    header[12] = uint8_t(w & 0xFF);
    header[13] = uint8_t((w >> 8) & 0xFF);
    header[14] = uint8_t(h & 0xFF);
    header[15] = uint8_t((h >> 8) & 0xFF);
    header[16] = 32; // bits per pixel
    header[17] = 0x28;

    std::ofstream out(path, std::ios::binary);
    out.write((const char*)header, sizeof(header));

    std::vector<uint8_t> pixels(std::size_t(w) * h * 4, 0xFF);
    out.write((const char*)pixels.data(), pixels.size());
}

/* Writes a TexturePacker-style "array" JSON atlas (matching the format
 * written by tools/sprite_gen/atlas_writer.cpp) describing two
 * deliberately non-grid-aligned frames on a 100x50 sheet, plus a single
 * "idle" animation covering both frames. The frame rects are picked so
 * that they can't be confused with the result of the old grid-based
 * (frame_width/frame_height) UV calculation. */
void write_spritesheet_test_atlas(const std::string& path,
                                  const std::string& image_filename) {
    std::ofstream out(path);
    out << R"({
        "frames": [
            {
                "filename": "frame_0",
                "frame": {"x": 4, "y": 6, "w": 20, "h": 10},
                "rotated": false,
                "trimmed": false,
                "sourceSize": {"w": 20, "h": 10},
                "spriteSourceSize": {"x": 0, "y": 0, "w": 20, "h": 10},
                "duration": 100
            },
            {
                "filename": "frame_1",
                "frame": {"x": 50, "y": 20, "w": 15, "h": 8},
                "rotated": false,
                "trimmed": false,
                "sourceSize": {"w": 15, "h": 8},
                "spriteSourceSize": {"x": 0, "y": 0, "w": 15, "h": 8},
                "duration": 100
            }
        ],
        "meta": {
            "image": ")" << image_filename << R"(",
            "size": {"w": 100, "h": 50},
            "scale": "1",
            "frameTags": [
                {"name": "idle", "from": 0, "to": 1, "direction": "forward"}
            ]
        }
    })";
}

class SpritesheetTests : public smlt::test::SimulantTestCase {
public:
    void set_up() override {
        smlt::test::SimulantTestCase::set_up();

        tmpdir_ = kfs::path::join(kfs::temp_dir(), "spritesheet_test");
        if(!kfs::path::exists(tmpdir_)) {
            kfs::make_dir(tmpdir_);
        }

        texture_path_ = kfs::path::join(tmpdir_, "sheet.tga");
        atlas_path_ = kfs::path::join(tmpdir_, "sheet.json");

        write_spritesheet_test_texture(texture_path_, 100, 50);
        write_spritesheet_test_atlas(atlas_path_, "sheet.tga");

        vfs_ = application->vfs.get();
        vfs_->add_search_path(tmpdir_);
    }

    void tear_down() override {
        vfs_->remove_search_path(tmpdir_);
        std::remove(texture_path_.c_str());
        std::remove(atlas_path_.c_str());

        smlt::test::SimulantTestCase::tear_down();
    }

    void test_load_parses_frames() {
        auto sheet = scene->assets->load_spritesheet("sheet.json");
        assert_is_not_null(sheet.get());

        assert_equal(sheet->frame_count(), (std::size_t)2);

        auto* frame0 = sheet->frame(0);
        assert_is_not_null(frame0);
        assert_equal(frame0->name, std::string("frame_0"));
        assert_equal(frame0->x, (uint16_t)4);
        assert_equal(frame0->y, (uint16_t)6);
        assert_equal(frame0->w, (uint16_t)20);
        assert_equal(frame0->h, (uint16_t)10);
        assert_equal(frame0->duration_ms, (uint32_t)100);

        auto* frame1 = sheet->frame(1);
        assert_is_not_null(frame1);
        assert_equal(frame1->name, std::string("frame_1"));
        assert_equal(frame1->x, (uint16_t)50);
        assert_equal(frame1->y, (uint16_t)20);

        assert_is_null(sheet->frame(2));
    }

    void test_load_parses_frame_tags_as_animations() {
        auto sheet = scene->assets->load_spritesheet("sheet.json");
        assert_is_not_null(sheet.get());

        assert_equal(sheet->animation_count(), (std::size_t)1);

        auto* anim = sheet->animation(0);
        assert_is_not_null(anim);
        assert_equal(anim->name, std::string("idle"));
        assert_equal(anim->start_frame, (uint32_t)0);
        assert_equal(anim->end_frame, (uint32_t)1);
    }

    void test_load_resolves_relative_texture() {
        auto sheet = scene->assets->load_spritesheet("sheet.json");
        assert_is_not_null(sheet.get());

        auto texture = sheet->texture();
        assert_is_not_null(texture.get());
        assert_equal(texture->width(), (uint16_t)100);
        assert_equal(texture->height(), (uint16_t)50);
    }

    void test_find_frame_by_name() {
        auto sheet = scene->assets->load_spritesheet("sheet.json");
        assert_is_not_null(sheet.get());

        auto found = sheet->find_frame("frame_1");
        assert_true(bool(found));
        assert_equal(found.value(), (std::size_t)1);

        auto missing = sheet->find_frame("nope");
        assert_false(bool(missing));
    }

    void test_load_missing_file_returns_null() {
        auto sheet = scene->assets->load_spritesheet("does_not_exist.json");
        assert_is_null(sheet.get());
    }

    void test_sprite_set_spritesheet_applies_frame_uvs() {
        auto sheet = scene->assets->load_spritesheet("sheet.json");
        assert_is_not_null(sheet.get());

        auto sprite = scene->create_child<Sprite>();
        sprite->set_spritesheet(sheet);

        assert_true(sprite->has_animations());

        /* The current frame defaults to 0, so the mesh should immediately
         * be textured using frame_0's rect (4, 6, 20, 10) on the 100x50
         * sheet - not the old grid-based calculation, which would (for
         * frame index 0, with no margin/spacing/padding set) produce
         * (0, 0) instead. */
        auto mesh = sprite->actor->base_mesh();
        assert_is_not_null(mesh.get());

        auto* uv0 = mesh->vertex_data->texcoord0_at<Vec2>(0);
        assert_is_not_null(uv0);
        assert_close(uv0->x, 0.045f, 0.001f);
        assert_close(uv0->y, 0.13f, 0.001f);

        auto* uv1 = mesh->vertex_data->texcoord0_at<Vec2>(1);
        assert_is_not_null(uv1);
        assert_close(uv1->x, 0.235f, 0.001f);
        assert_close(uv1->y, 0.13f, 0.001f);
    }

    void test_sprite_creation_param_wires_spritesheet() {
        auto sheet = scene->assets->load_spritesheet("sheet.json");
        assert_is_not_null(sheet.get());

        auto sprite =
            scene->create_child<Sprite>(Params().set("spritesheet", sheet));

        assert_true(sprite->has_animations());
        assert_is_not_null(sprite->material().get());

        /* Playing the animation shouldn't crash and should leave the
         * sprite on a valid frame. */
        sprite->animations->play_animation("idle");
        sprite->update(0.016f);
    }

    void test_sprite_creation_without_spritesheet_param_still_works() {
        auto sprite = scene->create_child<Sprite>();
        assert_false(sprite->has_animations());
    }

private:
    std::string tmpdir_;
    std::string texture_path_;
    std::string atlas_path_;
    VirtualFileSystem* vfs_ = nullptr;
};

} // namespace
