/* Simulant benchmark sample
 *
 * "SECTOR" - an automated first-person flythrough of a procedurally
 * generated maze. It exercises meshes (a large merged static level),
 * skinned + node-animated prefabs, particle systems, 3D/ambient audio,
 * dynamic lights, stencil shadows, a skybox and the widget/HUD path.
 *
 * It runs for RUN_SECONDS, printing frame statistics to stdout every
 * second, then exits cleanly with a profiling summary (and a per-frame
 * CSV on desktop).
 *
 * Run headless / without audio while iterating:
 *   SIMULANT_HIDDEN_WINDOW=1 SIMULANT_SOUND_DRIVER=null ./benchmark
 * Force uncapped profiling:
 *   SIMULANT_PROFILE=1 ./benchmark
 */

#include "simulant/simulant.h"
#include "simulant/nodes/frustum_culler.h"
#include "simulant/tools/profiler.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#ifdef __DREAMCAST__
#include <dc/video.h>
#include "simulant/platforms/dreamcast/profiler.h"
#endif
#include <deque>
#include <fstream>
#include <random>
#include <string>
#include <vector>

using namespace smlt;

namespace {

constexpr int MAZE_W = 23;
constexpr int MAZE_H = 23;
constexpr float CELL = 4.0f;
constexpr float WALL_H = 3.2f;
constexpr float WALL_T = 0.5f;
constexpr float PILLAR_T = 0.9f;
constexpr float FLOOR_Y = 0.0f;
constexpr float CAM_HEIGHT = 1.55f;
constexpr float WALK_SPEED = 7.5f;
constexpr float CAM_LOOKAHEAD = 3.5f;
constexpr float RUN_SECONDS = 15.0f;
constexpr float TEX_TILE = 2.0f;
constexpr int CHUNK = 6;
constexpr float EXPLOSION_INTERVAL = 2.5f;
constexpr int WARMUP_FRAMES = 20;

constexpr uint8_t WALL_N = 0x1;
constexpr uint8_t WALL_E = 0x2;
constexpr uint8_t WALL_S = 0x4;
constexpr uint8_t WALL_W = 0x8;

inline int cell_index(int x, int z) {
    return z * MAZE_W + x;
}

Vec3 cell_center(int x, int z) {
    const float cw = (MAZE_W - 1) * 0.5f;
    const float ch = (MAZE_H - 1) * 0.5f;
    return Vec3(float(x) - cw, 0.0f, float(z) - ch) * CELL;
}

Vec3 catmull_rom(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3,
                 float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return ((p1 * 2.0f) + (p2 - p0) * t +
            (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
            (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) *
           0.5f;
}

Vec3 catmull_rom_tangent(const Vec3& p0, const Vec3& p1, const Vec3& p2,
                         const Vec3& p3, float t) {
    const float t2 = t * t;
    return ((p2 - p0) +
            (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * (2.0f * t) +
            (p1 * 3.0f - p0 - p2 * 3.0f + p3) * (3.0f * t2)) *
           0.5f;
}

/* Emits a single textured box, matching the winding/normal convention used by
 * Mesh::create_submesh_as_box so that back-face culling works correctly. */
void append_box(VertexData* vd, IndexData* id, const Vec3& c, const Vec3& size,
                const Color& color, float tile) {
    const float hx = size.x * 0.5f;
    const float hy = size.y * 0.5f;
    const float hz = size.z * 0.5f;

    auto face = [&](const Vec3& n, const Vec3 p[4], float uw, float vh,
                    bool flip) {
        const uint32_t base = vd->count();
        const Vec2 uv[4] = {Vec2(0, 0), Vec2(uw, 0), Vec2(uw, vh), Vec2(0, vh)};
        for(int i = 0; i < 4; ++i) {
            vd->position(c + p[i]);
            vd->normal(n);
            vd->tex_coord0(uv[i]);
            vd->color(color);
            vd->move_next();
        }
        if(!flip) {
            id->index(base);
            id->index(base + 1);
            id->index(base + 2);
            id->index(base);
            id->index(base + 2);
            id->index(base + 3);
        } else {
            id->index(base);
            id->index(base + 2);
            id->index(base + 1);
            id->index(base);
            id->index(base + 3);
            id->index(base + 2);
        }
    };

    const float ux = (2.0f * hx) / tile;
    const float uy = (2.0f * hy) / tile;
    const float uz = (2.0f * hz) / tile;

    {
        Vec3 p[4] = {{-hx, -hy, hz}, {hx, -hy, hz}, {hx, hy, hz}, {-hx, hy, hz}};
        face(Vec3(0, 0, 1), p, ux, uy, false);
    }
    {
        Vec3 p[4] = {
            {-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {-hx, hy, -hz}};
        face(Vec3(0, 0, -1), p, ux, uy, true);
    }
    {
        Vec3 p[4] = {{hx, -hy, -hz}, {hx, hy, -hz}, {hx, hy, hz}, {hx, -hy, hz}};
        face(Vec3(1, 0, 0), p, uz, uy, false);
    }
    {
        Vec3 p[4] = {
            {-hx, -hy, -hz}, {-hx, hy, -hz}, {-hx, hy, hz}, {-hx, -hy, hz}};
        face(Vec3(-1, 0, 0), p, uz, uy, true);
    }
    {
        Vec3 p[4] = {{hx, hy, -hz}, {-hx, hy, -hz}, {-hx, hy, hz}, {hx, hy, hz}};
        face(Vec3(0, 1, 0), p, ux, uz, false);
    }
    {
        Vec3 p[4] = {
            {hx, -hy, -hz}, {-hx, -hy, -hz}, {-hx, -hy, hz}, {hx, -hy, hz}};
        face(Vec3(0, -1, 0), p, ux, uz, true);
    }
}

void append_floor_quad(VertexData* vd, IndexData* id, float cx, float cz,
                       float half, const Color& color, float tile) {
    const uint32_t base = vd->count();
    const float u = (2.0f * half) / tile;
    vd->position(Vec3(cx + half, FLOOR_Y, cz - half));
    vd->normal(Vec3(0, 1, 0));
    vd->tex_coord0(0, 0);
    vd->color(color);
    vd->move_next();

    vd->position(Vec3(cx - half, FLOOR_Y, cz - half));
    vd->normal(Vec3(0, 1, 0));
    vd->tex_coord0(u, 0);
    vd->color(color);
    vd->move_next();

    vd->position(Vec3(cx - half, FLOOR_Y, cz + half));
    vd->normal(Vec3(0, 1, 0));
    vd->tex_coord0(u, u);
    vd->color(color);
    vd->move_next();

    vd->position(Vec3(cx + half, FLOOR_Y, cz + half));
    vd->normal(Vec3(0, 1, 0));
    vd->tex_coord0(0, u);
    vd->color(color);
    vd->move_next();

    id->index(base);
    id->index(base + 1);
    id->index(base + 2);
    id->index(base);
    id->index(base + 2);
    id->index(base + 3);
}

} // namespace

class BenchmarkScene: public Scene {
public:
    BenchmarkScene(Window* window):
        Scene(window) {}

    void on_load() override {
        rng_.seed(0xC0FFEE);

        world_ = create_child<Stage>();
        hud_ = create_child<Stage>();

        setup_camera_and_layers();
        load_assets();
        ram_check("after assets");

        generate_maze();
        solve_maze();
        build_camera_curve();
        build_level();
        ram_check("after level");

        spawn_props();
        spawn_animated();
        spawn_torches();
        setup_sounds();
        setup_hud();
        ram_check("after scene setup");

        get_app()->signal_frame_finished().connect(
            [this]() { on_frame_finished(); });
        get_app()->signal_pre_swap().connect([this]() {
            if(!ram_reported_first_frame_) {
                ram_reported_first_frame_ = true;
                ram_check("first frame");
            }
        });

        if(const char* capture = std::getenv("SIMULANT_BENCH_CAPTURE")) {
            capture_path_ = capture;
            get_app()->signal_pre_swap().connect([this]() { maybe_capture(); });
        }

        print_scene_report();
    }

    void on_update(float dt) override {
        if(finished_) {
            return;
        }
        _S_PROFILE_SECTION("benchmark/update");
        update_camera(dt);
        update_torches(dt);
        update_spawners(dt);
    }

private:
    /* ------------------------------------------------------------------ */
    /* Setup                                                              */
    /* ------------------------------------------------------------------ */

    void setup_camera_and_layers() {
        camera_ = create_child<Camera3D>();
        camera_->set_perspective_projection(
            Degrees(70.0f), float(window->width()) / float(window->height()),
            0.05f, 400.0f);

        auto layer = compositor->create_layer(world_, camera_);
        layer->set_clear_flags(BUFFER_CLEAR_ALL);
        layer->viewport->set_color(Color(0.03f, 0.04f, 0.06f, 1.0f));

#ifndef __DREAMCAST__
        /* The Dreamcast has very little VRAM; skip the cubemap there. */
        auto sky = world_->create_child<Skybox>(
            "assets/samples/skyboxes/TropicalSunnyDay");
        sky->set_size(500.0f);
#endif

        lighting->set_ambient_light(Color(0.20f, 0.21f, 0.25f, 1.0f));

        sun_ = world_->create_child<DirectionalLight>();
        sun_->set_direction(Vec3(-0.4f, -1.0f, 0.25f).normalized());
        sun_->set_color(Color(1.0f, 0.93f, 0.78f, 1.0f));
        sun_->set_intensity(0.9f);

        /* A cool point light that rides along with the camera. */
        headlamp_ = world_->create_child<PointLight>();
        headlamp_->set_color(Color(0.5f, 0.7f, 1.0f, 1.0f));
        headlamp_->set_range(18.0f);
        /* Intensities on this path multiply directly into the lighting term
         * (there is no inverse-square falloff), so point lights use the same
         * ~1 scale as the sun. */
        headlamp_->set_intensity(0.45f);
    }

    void load_assets() {
        wall_tex_ = assets->load_texture("assets/samples/bench/wall.png");
        floor_tex_ =
            assets->load_texture("assets/samples/bench/floor.png");
        crate_tex_ = assets->load_texture("assets/samples/crate.png");

        wall_tex_->set_texture_filter(TEXTURE_FILTER_BILINEAR);
        floor_tex_->set_texture_filter(TEXTURE_FILTER_BILINEAR);
        crate_tex_->set_texture_filter(TEXTURE_FILTER_BILINEAR);

        fire_script_ = assets->load_particle_script(ParticleScript::BuiltIns::FIRE);
        explosion_script_ =
            assets->load_particle_script("particles/pixel_explosion.kglp");
        trail_script_ =
            assets->load_particle_script("particles/pixel_trail.kglp");

        ambient_sound_ =
            assets->load_sound("assets/samples/cave/ambient.wav");
        explosion_sound_ =
            assets->load_sound("assets/samples/bench/explosion.wav");

        cube_prefab_ = assets->load_prefab("assets/samples/AnimatedCube.gltf");
        rig_prefab_ =
            assets->load_prefab("assets/samples/khronos/RiggedSimple.glb");
    }

    /* ------------------------------------------------------------------ */
    /* Procedural maze                                                    */
    /* ------------------------------------------------------------------ */

    void generate_maze() {
        walls_.assign(MAZE_W * MAZE_H, WALL_N | WALL_E | WALL_S | WALL_W);
        std::vector<bool> visited(MAZE_W * MAZE_H, false);
        std::vector<int> stack;
        stack.reserve(MAZE_W * MAZE_H);
        visited[0] = true;
        stack.push_back(0);

        while(!stack.empty()) {
            const int c = stack.back();
            const int x = c % MAZE_W;
            const int z = c / MAZE_W;

            int nbrs[4];
            uint8_t dir[4];
            uint8_t opp[4];
            int n = 0;

            if(z > 0 && !visited[c - MAZE_W]) {
                nbrs[n] = c - MAZE_W;
                dir[n] = WALL_N;
                opp[n] = WALL_S;
                ++n;
            }
            if(x < MAZE_W - 1 && !visited[c + 1]) {
                nbrs[n] = c + 1;
                dir[n] = WALL_E;
                opp[n] = WALL_W;
                ++n;
            }
            if(z < MAZE_H - 1 && !visited[c + MAZE_W]) {
                nbrs[n] = c + MAZE_W;
                dir[n] = WALL_S;
                opp[n] = WALL_N;
                ++n;
            }
            if(x > 0 && !visited[c - 1]) {
                nbrs[n] = c - 1;
                dir[n] = WALL_W;
                opp[n] = WALL_E;
                ++n;
            }

            if(n == 0) {
                stack.pop_back();
                continue;
            }

            const int pick = std::uniform_int_distribution<int>(0, n - 1)(rng_);
            walls_[c] &= ~dir[pick];
            walls_[nbrs[pick]] &= ~opp[pick];
            visited[nbrs[pick]] = true;
            stack.push_back(nbrs[pick]);
        }
    }

    void solve_maze() {
        const int count = MAZE_W * MAZE_H;
        std::vector<int> parent(count, -1);
        std::deque<int> queue;
        const int start = 0;
        const int end = count - 1;
        parent[start] = -2;
        queue.push_back(start);

        while(!queue.empty()) {
            const int c = queue.front();
            queue.pop_front();
            if(c == end) {
                break;
            }
            const int x = c % MAZE_W;
            const int z = c / MAZE_W;

            if(z > 0 && !(walls_[c] & WALL_N) && parent[c - MAZE_W] == -1) {
                parent[c - MAZE_W] = c;
                queue.push_back(c - MAZE_W);
            }
            if(x < MAZE_W - 1 && !(walls_[c] & WALL_E) && parent[c + 1] == -1) {
                parent[c + 1] = c;
                queue.push_back(c + 1);
            }
            if(z < MAZE_H - 1 && !(walls_[c] & WALL_S) &&
               parent[c + MAZE_W] == -1) {
                parent[c + MAZE_W] = c;
                queue.push_back(c + MAZE_W);
            }
            if(x > 0 && !(walls_[c] & WALL_W) && parent[c - 1] == -1) {
                parent[c - 1] = c;
                queue.push_back(c - 1);
            }
        }

        path_.clear();
        for(int c = end; c != -2 && c != -1; c = parent[c]) {
            path_.push_back(c);
        }
        std::reverse(path_.begin(), path_.end());
    }

    void build_camera_curve() {
        std::vector<Vec3> controls;
        controls.reserve(path_.size());
        for(int c: path_) {
            controls.push_back(cell_center(c % MAZE_W, c / MAZE_W) +
                                Vec3(0, CAM_HEIGHT, 0));
        }
        if(controls.size() < 2) {
            return;
        }

        cam_points_.clear();
        cam_tangents_.clear();
        const int sub = 16;
        for(std::size_t i = 0; i + 1 < controls.size(); ++i) {
            const Vec3& p0 = controls[i == 0 ? 0 : i - 1];
            const Vec3& p1 = controls[i];
            const Vec3& p2 = controls[i + 1];
            const Vec3& p3 = controls[std::min(i + 2, controls.size() - 1)];
            for(int s = 0; s < sub; ++s) {
                const float t = float(s) / sub;
                cam_points_.push_back(catmull_rom(p0, p1, p2, p3, t));
                Vec3 tangent = catmull_rom_tangent(p0, p1, p2, p3, t);
                if(tangent.length_squared() > 0.0001f) {
                    tangent = tangent.normalized();
                }
                cam_tangents_.push_back(tangent);
            }
        }
        cam_points_.push_back(controls.back());
        {
            Vec3 tangent = controls.back() - controls[controls.size() - 2];
            if(tangent.length_squared() > 0.0001f) {
                tangent = tangent.normalized();
            }
            cam_tangents_.push_back(tangent);
        }

        cam_arc_.clear();
        cam_arc_.push_back(0.0f);
        for(std::size_t i = 1; i < cam_points_.size(); ++i) {
            cam_arc_.push_back(cam_arc_.back() +
                               (cam_points_[i] - cam_points_[i - 1]).length());
        }
        cam_length_ = cam_arc_.back();
    }

    /* ------------------------------------------------------------------ */
    /* Static level mesh                                                  */
    /* ------------------------------------------------------------------ */

    void build_level() {
        /* Parent the level chunk actors under a FrustumCuller so chunks
         * outside the camera frustum are never transformed, lit or submitted.
         * Without this every one of the 16 chunks is submitted every frame. */
        level_culler_ = world_->create_child<FrustumCuller>();

        /* The default material is BLEND_ALPHA on every renderer. On the PVR
         * that forces all geometry into the translucent (autosorted) list,
         * which cannot hold a level this size. These surfaces are opaque, so
         * mark them BLEND_NONE to route them to the opaque list. */
        auto wall_mat = assets->clone_default_material();
        wall_mat->set_base_color_map(wall_tex_);
        wall_mat->set_lighting_enabled(true);
        wall_mat->set_cull_mode(CULL_MODE_BACK_FACE);
        wall_mat->set_blend_func(BLEND_NONE);

        auto floor_mat = assets->clone_default_material();
        floor_mat->set_base_color_map(floor_tex_);
        floor_mat->set_lighting_enabled(true);
        floor_mat->set_cull_mode(CULL_MODE_BACK_FACE);
        floor_mat->set_blend_func(BLEND_NONE);

        const Color wall_col = Color::white();
        const Color pillar_col = Color(0.82f, 0.85f, 0.90f, 1.0f);
        const float half = CELL * 0.5f;

        const int chunks_x = (MAZE_W + CHUNK - 1) / CHUNK;
        const int chunks_z = (MAZE_H + CHUNK - 1) / CHUNK;

        wall_vertices_ = 0;
        floor_vertices_ = 0;

        /* The level is split into chunk-sized meshes so that the per-object
         * light selection (max 8 desktop / 2 Dreamcast) picks sensible lights
         * for each region instead of lighting the whole maze from its center.
         * This also gives the renderer many more draw calls to chew on. */
        for(int cz = 0; cz < chunks_z; ++cz) {
            for(int cx = 0; cx < chunks_x; ++cx) {
                const int x0 = cx * CHUNK;
                const int x1 = std::min(x0 + CHUNK, MAZE_W);
                const int z0 = cz * CHUNK;
                const int z1 = std::min(z0 + CHUNK, MAZE_H);

                auto mesh = assets->create_mesh(VertexSpecification::DEFAULT,
                                                GARBAGE_COLLECT_NEVER);
                auto wall_sub = mesh->create_submesh("walls", wall_mat,
                                                     INDEX_TYPE_16_BIT);
                auto floor_sub = mesh->create_submesh("floor", floor_mat,
                                                      INDEX_TYPE_16_BIT);
                VertexData* vd = mesh->vertex_data;
                IndexData* wid = wall_sub->index_data;
                IndexData* fid = floor_sub->index_data;

                /* One box per wall, only emitting N/W plus the far S/E
                 * boundary so shared interior walls are produced once. */
                for(int z = z0; z < z1; ++z) {
                    for(int x = x0; x < x1; ++x) {
                        const int c = cell_index(x, z);
                        const Vec3 center = cell_center(x, z);

                        if(walls_[c] & WALL_N) {
                            append_box(vd, wid,
                                       center + Vec3(0, WALL_H * 0.5f, -half),
                                       Vec3(CELL + WALL_T, WALL_H, WALL_T),
                                       wall_col, TEX_TILE);
                        }
                        if(walls_[c] & WALL_W) {
                            append_box(vd, wid,
                                       center + Vec3(-half, WALL_H * 0.5f, 0),
                                       Vec3(WALL_T, WALL_H, CELL + WALL_T),
                                       wall_col, TEX_TILE);
                        }
                        if(x == MAZE_W - 1 && (walls_[c] & WALL_E)) {
                            append_box(vd, wid,
                                       center + Vec3(half, WALL_H * 0.5f, 0),
                                       Vec3(WALL_T, WALL_H, CELL + WALL_T),
                                       wall_col, TEX_TILE);
                        }
                        if(z == MAZE_H - 1 && (walls_[c] & WALL_S)) {
                            append_box(vd, wid,
                                       center + Vec3(0, WALL_H * 0.5f, half),
                                       Vec3(CELL + WALL_T, WALL_H, WALL_T),
                                       wall_col, TEX_TILE);
                        }
                    }
                }

                /* A pillar at every grid intersection seals the corners. */
                for(int z = z0; z <= z1; ++z) {
                    for(int x = x0; x <= x1; ++x) {
                        const int pcx = std::min(x, MAZE_W - 1) / CHUNK;
                        const int pcz = std::min(z, MAZE_H - 1) / CHUNK;
                        if(pcx != cx || pcz != cz) {
                            continue;
                        }
                        const float px = (float(x) - MAZE_W * 0.5f) * CELL;
                        const float pz = (float(z) - MAZE_H * 0.5f) * CELL;
                        append_box(vd, wid,
                                   Vec3(px, (WALL_H + 0.25f) * 0.5f, pz),
                                   Vec3(PILLAR_T, WALL_H + 0.25f, PILLAR_T),
                                   pillar_col, 1.0f);
                    }
                }

                const uint32_t after_walls = vd->count();

                for(int z = z0; z < z1; ++z) {
                    for(int x = x0; x < x1; ++x) {
                        const Vec3 center = cell_center(x, z);
                        const bool alt = ((x + z) % 2) == 0;
                        const Color col =
                            alt ? Color(1.0f, 1.0f, 1.0f, 1.0f)
                                : Color(0.72f, 0.78f, 0.88f, 1.0f);
                        append_floor_quad(vd, fid, center.x, center.z, half,
                                          col, TEX_TILE);
                    }
                }

                vd->done();
                wid->done();
                fid->done();

                wall_vertices_ += after_walls;
                floor_vertices_ += (vd->count() - after_walls);
                level_culler_->create_child<Actor>(mesh);
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* Props / animated actors                                            */
    /* ------------------------------------------------------------------ */

    void spawn_props() {
        auto crate_mat = assets->clone_default_material();
        crate_mat->set_base_color_map(crate_tex_);
        crate_mat->set_lighting_enabled(true);
        crate_mat->set_blend_func(BLEND_NONE);

        crate_mesh_ = assets->create_mesh(VertexSpecification::DEFAULT,
                                          GARBAGE_COLLECT_NEVER);
        crate_mesh_->create_submesh_as_box("crate", crate_mat, 1.3f, 1.3f, 1.3f);

        barrel_mesh_ = assets->create_mesh(VertexSpecification::DEFAULT,
                                           GARBAGE_COLLECT_NEVER);
        barrel_mesh_->create_submesh_as_cylinder("barrel", crate_mat, 0.9f, 1.4f,
                                                 12, 1);

        auto orb_mat = assets->clone_default_material();
        orb_mat->set_base_color_map(crate_tex_);
        orb_mat->set_lighting_enabled(false);
        orb_mat->set_blend_func(BlendType::BLEND_ADD);

        orb_mesh_ = assets->create_mesh(VertexSpecification::DEFAULT,
                                        GARBAGE_COLLECT_NEVER);
        orb_mesh_->create_submesh_as_sphere("orb", orb_mat, 0.7f, 8, 6);

        std::uniform_real_distribution<float> jitter(-1.1f, 1.1f);
        std::uniform_real_distribution<float> yaw(0.0f, 360.0f);

        for(int i = 0; i < 70; ++i) {
            const int c = std::uniform_int_distribution<int>(
                0, MAZE_W * MAZE_H - 1)(rng_);
            const int x = c % MAZE_W;
            const int z = c / MAZE_W;
            Vec3 p = cell_center(x, z) + Vec3(jitter(rng_), 0, jitter(rng_));
            p.y = 0.65f;

            auto actor = world_->create_child<Actor>(crate_mesh_);
            actor->transform->set_position(p);
            actor->transform->set_rotation(
                Quaternion(Degrees(0), Degrees(yaw(rng_)), Degrees(0)));

            if((i % 5) == 0) {
                auto stacked = world_->create_child<Actor>(crate_mesh_);
                stacked->transform->set_position(p + Vec3(0, 1.3f, 0));
                stacked->transform->set_rotation(
                    Quaternion(Degrees(0), Degrees(yaw(rng_)), Degrees(0)));
            }
        }

        for(int i = 0; i < 24; ++i) {
            const int c = std::uniform_int_distribution<int>(
                0, MAZE_W * MAZE_H - 1)(rng_);
            auto actor =
                world_->create_child<Actor>(barrel_mesh_);
            actor->transform->set_position(
                cell_center(c % MAZE_W, c / MAZE_W) +
                Vec3(jitter(rng_), 0.7f, jitter(rng_)));
        }

        for(int i = 0; i < 12; ++i) {
            const int c = std::uniform_int_distribution<int>(
                0, MAZE_W * MAZE_H - 1)(rng_);
            auto orb = world_->create_child<Actor>(orb_mesh_);
            orb->set_render_priority(RENDER_PRIORITY_NEAR);
            orb->transform->set_position(
                cell_center(c % MAZE_W, c / MAZE_W) + Vec3(0, 1.6f, 0));
            orbs_.push_back(orb);
        }
    }

    void spawn_animated() {
#ifndef __DREAMCAST__
        /* Stencil shadow volumes are GL-only today. SIMULANT_BENCH_NO_SHADOWS is a
         * debug escape hatch. */
        if(!std::getenv("SIMULANT_BENCH_NO_SHADOWS")) {
            shadows_ = world_->create_child<ShadowCaster>();
        }
#endif

        if(cube_prefab_) {
            for(int i = 0; i < 12; ++i) {
                const int c = std::uniform_int_distribution<int>(
                    0, MAZE_W * MAZE_H - 1)(rng_);
                const Vec3 p =
                    cell_center(c % MAZE_W, c / MAZE_W) +
                    Vec3(0, 1.4f + 0.2f * (i % 4), 0);

                auto inst = world_->create_child<PrefabInstance>(cube_prefab_);
                inst->transform->set_position(p);
                inst->transform->set_scale(Vec3(0.9f, 0.9f, 0.9f));

                auto ctrl = inst->find_mixin<AnimationController>();
                if(ctrl) {
                    auto names = ctrl->animation_names();
                    if(!names.empty()) {
                        ctrl->play(names[0], -1);
                        ctrl->set_animation_speed(0.8f + 0.05f * (i % 5));
                    }
                }

                drones_.push_back(inst);
                drone_phase_.push_back(float(i) * 0.7f);

                if((i % 3) == 0 && trail_script_) {
                    auto trail = inst->create_child<ParticleSystem>(trail_script_);
                    trail->set_space(PARTICLE_SYSTEM_SPACE_LOCAL);
                    trail_systems_.push_back(trail);
                }
            }
        }

        if(rig_prefab_) {
            for(int i = 0; i < 8; ++i) {
                const int c = std::uniform_int_distribution<int>(
                    0, MAZE_W * MAZE_H - 1)(rng_);
                const Vec3 p = cell_center(c % MAZE_W, c / MAZE_W) +
                               Vec3(0, 1.0f, 0);

                auto inst = world_->create_child<PrefabInstance>(rig_prefab_);
                inst->transform->set_position(p);
#ifndef __DREAMCAST__
                if(shadows_) {
                    inst->set_parent(shadows_);
                }
#endif

                auto found = inst->find_descendents_by_types(
                    {Armature::Meta::node_type});
                if(!found.empty()) {
                    auto armature = static_cast<ArmaturePtr>(found[0]);
                    if(armature->skinned_mesh()) {
                        auto m = armature->skinned_mesh()
                                     ->first_submesh()
                                     ->material();
                        m->set_lighting_enabled(true);
                        m->set_blend_func(BLEND_NONE);
                    }
                }

                auto ctrl = inst->find_mixin<AnimationController>();
                if(ctrl) {
                    auto names = ctrl->animation_names();
                    if(!names.empty()) {
                        ctrl->play(names[0], -1);
                        ctrl->set_animation_speed(1.0f + 0.1f * (i % 4));
                    }
                }
                sentinels_.push_back(inst);
            }
        }
    }

    void spawn_torches() {
        if(!fire_script_ || path_.empty()) {
            return;
        }

        std::uniform_int_distribution<int> dir_pick(0, 3);
        for(std::size_t i = 2; i < path_.size(); i += std::max<std::size_t>(
                                                   1, path_.size() / 12)) {
            const int c = path_[i];
            const int x = c % MAZE_W;
            const int z = c / MAZE_W;
            Vec3 p = cell_center(x, z);

            /* Nudge the torch toward a wall if there is one. */
            const int d = dir_pick(rng_);
            if(d == 0 && (walls_[c] & WALL_N)) {
                p.z -= CELL * 0.5f - 0.45f;
            } else if(d == 1 && (walls_[c] & WALL_E)) {
                p.x += CELL * 0.5f - 0.45f;
            } else if(d == 2 && (walls_[c] & WALL_S)) {
                p.z += CELL * 0.5f - 0.45f;
            } else if(d == 3 && (walls_[c] & WALL_W)) {
                p.x -= CELL * 0.5f - 0.45f;
            }
            p.y = 1.25f;

            auto fire = world_->create_child<ParticleSystem>(fire_script_);
            fire->transform->set_position(p);
            fire_systems_.push_back(fire);

            auto light = world_->create_child<PointLight>();
            light->set_color(Color(1.0f, 0.55f, 0.15f, 1.0f));
            light->set_range(12.0f);
            light->set_intensity(2.5f);
            light->transform->set_position(p + Vec3(0, 0.4f, 0));
            torch_lights_.push_back(light);
            torch_base_intensity_.push_back(2.5f);
        }
    }

    void setup_sounds() {
#ifdef __DREAMCAST__
        /* Bring-up: streaming audio over the dcload serial fileserver appears
         * to stall the main loop on real hardware. Re-enable once the audio
         * streaming path is proven safe under serial I/O contention. */
        if(std::getenv("SIMULANT_BENCH_AUDIO") == nullptr) {
            return;
        }
#endif
        ambient_source_ = world_->create_child<AudioSource>();
        if(ambient_sound_) {
            ambient_playing_ = ambient_source_->play_sound(
                ambient_sound_, AUDIO_REPEAT_FOREVER, DISTANCE_MODEL_AMBIENT);
            if(ambient_playing_) {
                ambient_playing_->set_gain(0.35f);
            }
        }

        for(int i = 0; i < 4; ++i) {
            sfx_sources_.push_back(world_->create_child<AudioSource>());
        }
    }

    void setup_hud() {
        panel_cam_ = hud_->create_child<Camera2D>();
        panel_cam_->set_orthographic_projection(0, window->width(), 0,
                                                window->height());

        stats_panel_ = hud_->create_child<StatsPanel>();
        stats_panel_->activate();
        /* The stock StatsPanel draws its first line at height - 32 which clips
         * against the top edge; nudge the whole panel down a little. */
        stats_panel_->transform->set_position_2d(Vec2(0, -40));

        hud_label_ = hud_->create_child<ui::Label>("");
        hud_label_->set_anchor_point(0.0f, 1.0f);
        hud_label_->set_background_color(Color(0.0f, 0.0f, 0.0f, 0.55f));
        hud_label_->set_padding(6);
        hud_label_->transform->set_position_2d(
            window->coordinate_from_normalized(0.60f, 0.95f));

        hud_bar_ = hud_->create_child<ui::ProgressBar>();
        hud_bar_->resize(320, -1);
        hud_bar_->set_text("Benchmark progress");
        hud_bar_->set_anchor_point(0.0f, 1.0f);
        hud_bar_->transform->set_position_2d(
            window->coordinate_from_normalized(0.60f, 0.80f));

        auto layer = compositor->create_layer(hud_, panel_cam_);
        layer->set_clear_flags(0);
    }

    /* ------------------------------------------------------------------ */
    /* Per-frame updates                                                  */
    /* ------------------------------------------------------------------ */

    void update_camera(float dt) {
        if(cam_length_ <= 0.0f) {
            return;
        }

        walk_distance_ += WALK_SPEED * dt;
        walk_distance_ = std::fmod(walk_distance_, cam_length_);

        Vec3 pos;
        Vec3 tangent;
        sample_camera_path(walk_distance_, pos, tangent);

        walk_phase_ += dt * 9.0f;
        walk_time_ += dt;
        pos.y += std::sin(walk_phase_) * 0.05f;

        camera_->transform->set_position(pos);

        /* Look at a point further down the path rather than at the immediate
         * tangent; this averages out curvature and keeps the heading smooth
         * through corners. */
        Vec3 ahead_pos;
        Vec3 ahead_tangent;
        sample_camera_path(
            std::fmod(walk_distance_ + CAM_LOOKAHEAD, cam_length_), ahead_pos,
            ahead_tangent);

        Vec3 facing = ahead_pos - pos;
        facing.y = 0.0f;
        if(facing.length_squared() < 0.0001f) {
            facing = tangent;
        }

        const float yaw = std::sin(walk_time_ * 0.55f) * 0.10f;
        const Quaternion sway(Vec3::up(), Degrees(yaw * PI_UNDER_180));
        const Vec3 look = pos + (sway * facing);
        camera_->transform->look_at(look);
        camera_->transform->rotate(
            Vec3::forward(), Degrees(std::sin(walk_phase_ * 0.5f) * 0.5f));

        if(headlamp_) {
            headlamp_->transform->set_position(pos + Vec3(0, 0.1f, 0));
        }
    }

    void sample_camera_path(float distance, Vec3& pos, Vec3& tangent) const {
        if(cam_arc_.size() < 2) {
            return;
        }
        auto it = std::upper_bound(cam_arc_.begin(), cam_arc_.end(), distance);
        std::size_t b = std::size_t(it - cam_arc_.begin());
        if(b == 0) {
            b = 1;
        }
        if(b >= cam_points_.size()) {
            b = cam_points_.size() - 1;
        }
        const std::size_t a = b - 1;
        const float seg = cam_arc_[b] - cam_arc_[a];
        const float t = (seg > 0.0001f) ? (distance - cam_arc_[a]) / seg : 0.0f;
        pos = cam_points_[a] + (cam_points_[b] - cam_points_[a]) * t;

        /* Interpolate the (already-normalized) per-sample tangents so the
         * heading doesn't snap when crossing a sample boundary. */
        tangent = cam_tangents_[a] + (cam_tangents_[b] - cam_tangents_[a]) * t;
        if(tangent.length_squared() > 0.0001f) {
            tangent = tangent.normalized();
        }
    }

    void update_torches(float dt) {
        torch_time_ += dt;
        for(std::size_t i = 0; i < torch_lights_.size(); ++i) {
            const float flicker =
                0.82f + 0.18f * std::sin(torch_time_ * 17.0f + float(i) * 2.1f);
            torch_lights_[i]->set_intensity(torch_base_intensity_[i] * flicker);
        }

        for(std::size_t i = 0; i < drones_.size(); ++i) {
            const float phase = drone_phase_[i] + walk_time_ * 1.7f;
            drones_[i]->transform->set_position(
                drones_[i]->transform->position() +
                Vec3(0, std::sin(phase) * 0.35f * dt, 0));
        }

        for(std::size_t i = 0; i < orbs_.size(); ++i) {
            orbs_[i]->transform->set_rotation(Quaternion(
                Degrees(0), Degrees(walk_time_ * 40.0f + float(i) * 20.0f),
                Degrees(0)));
        }
    }

    void update_spawners(float dt) {
        if(!explosion_script_) {
            return;
        }

        explosion_timer_ += dt;
        if(explosion_timer_ < EXPLOSION_INTERVAL) {
            return;
        }
        explosion_timer_ -= EXPLOSION_INTERVAL;

        if(cam_length_ <= 0.0f) {
            return;
        }

        const float at =
            std::fmod(walk_distance_ + 14.0f, std::max(cam_length_, 0.001f));
        Vec3 p;
        Vec3 t;
        sample_camera_path(at, p, t);
        p.y = 1.1f;
        spawn_explosion(p);
    }

    void spawn_explosion(const Vec3& p) {
        _S_PROFILE_SECTION("benchmark/spawn_explosion");
        auto system = world_->create_child<ParticleSystem>(explosion_script_);
        system->transform->set_position(p);
        system->set_destroy_on_completion(true);
        explosion_count_++;

        if(explosion_sound_ && !sfx_sources_.empty()) {
            AudioSource* src = sfx_sources_[sfx_index_];
            sfx_index_ = (sfx_index_ + 1) % sfx_sources_.size();
            src->transform->set_position(p);
            auto playing =
                src->play_sound(explosion_sound_, AUDIO_REPEAT_NONE,
                                DISTANCE_MODEL_POSITIONAL);
            if(playing) {
                playing->set_gain(0.7f);
            }
            sound_count_++;
        }
    }

    /* ------------------------------------------------------------------ */
    /* Instrumentation                                                    */
    /* ------------------------------------------------------------------ */

    void on_frame_finished() {
        const uint64_t now = TimeKeeper::now_in_us();
        if(last_frame_us_ == 0) {
            last_frame_us_ = now;
            return;
        }
        const uint64_t delta_us = now - last_frame_us_;
        last_frame_us_ = now;

        if(frame_counter_ == 0) {
            const int64_t free_ram =
                (int64_t) get_platform()->available_ram_in_bytes();
            const int64_t free_vram =
                (int64_t) get_platform()->available_vram_in_bytes();
            std::printf("[bench] first frame: free_ram=%lld free_vram=%lld\n",
                        (long long) free_ram, (long long) free_vram);
            std::fflush(stdout);
        }
#ifdef __DREAMCAST__
        /* One-shot framebuffer dump so we can visually verify PVR output both
         * under an emulator and on real hardware. */
        if(frame_counter_ == 20 && !shot_taken_) {
            shot_taken_ = true;
            if(vid_screen_shot("/pc/bench_shot.ppm") == 0) {
                std::printf("[bench] wrote /pc/bench_shot\n");
            } else {
                std::printf("[bench] screenshot failed\n");
            }
            std::fflush(stdout);
        }
#endif

#ifdef __DREAMCAST__
        /* Discard the load-time/warm-up samples so gmon.out describes only the
         * measured steady-state render window. */
        if(frame_counter_ == WARMUP_FRAMES) {
            profiler_reset();
        }
#endif

        if(finished_ || frame_counter_ < WARMUP_FRAMES) {
            frame_counter_++;
            return;
        }
        frame_counter_++;

        const float ms = float(delta_us) / 1000.0f;
        const float secs = float(delta_us) / 1000000.0f;

        frame_ms_.push_back(ms);
        seconds_frames_.push_back(ms);
        seconds_polys_.push_back(float(get_app()->stats->polygons_rendered()));
        seconds_renderables_.push_back(
            float(get_app()->stats->subactors_rendered()));
        run_time_ += secs;
        second_accum_ += secs;

        if(second_accum_ >= 1.0f) {
            print_second_line();
        }

        update_hud();

        if(run_time_ >= RUN_SECONDS) {
            finish();
        }
    }

    void update_hud() {
        if(!hud_label_) {
            return;
        }
        char buffer[160];
        const double last_ms = seconds_frames_.empty()
                                   ? 0.0
                                   : double(seconds_frames_.back());
        const double last_fps = last_ms > 0.0 ? 1000.0 / last_ms : 0.0;
        snprintf(buffer, sizeof(buffer),
                      "SECTOR BENCHMARK\n"
                      "time   %5.2fs / %.0fs\n"
                      "fps    %5.1f   frame %5.2f ms\n"
                      "polys  %6u   draws %5u\n"
                      "parts  %5u",
                      double(run_time_), double(RUN_SECONDS), last_fps, last_ms,
                      (unsigned) get_app()->stats->polygons_rendered(),
                      (unsigned) get_app()->stats->subactors_rendered(),
                      (unsigned) live_particle_count());
        hud_label_->set_text(buffer);

        if(hud_bar_) {
            hud_bar_->set_value((run_time_ / RUN_SECONDS) * 100.0f);
        }
    }

    unsigned live_particle_count() const {
        unsigned total = 0;
        for(auto* ps: fire_systems_) {
            total += unsigned(ps->particle_count());
        }
        for(auto* ps: trail_systems_) {
            total += unsigned(ps->particle_count());
        }
        return total;
    }

    void print_second_line() {
        double sum = 0.0;
        float lo = 1e30f;
        float hi = 0.0f;
        for(float m: seconds_frames_) {
            sum += double(m);
            lo = std::min(lo, m);
            hi = std::max(hi, m);
        }
        double poly_sum = 0.0;
        double draw_sum = 0.0;
        for(float v: seconds_polys_) {
            poly_sum += double(v);
        }
        for(float v: seconds_renderables_) {
            draw_sum += double(v);
        }
        const std::size_t n = seconds_frames_.size();
        const double avg = n ? sum / double(n) : 0.0;
        const double fps = avg > 0.0 ? 1000.0 / avg : 0.0;

        std::printf(
            "[bench] t=%6.2fs frames=%5u fps=%7.2f frame=%7.3fms "
            "min=%7.3f max=%7.3f polys=%8.0f draws=%6.0f particles=%u\n",
            double(run_time_), (unsigned) n, fps, avg, n ? double(lo) : 0.0,
            n ? double(hi) : 0.0, n ? poly_sum / double(n) : 0.0,
            n ? draw_sum / double(n) : 0.0, live_particle_count());
        std::fflush(stdout);

        seconds_frames_.clear();
        seconds_polys_.clear();
        seconds_renderables_.clear();
        second_accum_ = 0.0f;
    }

    void ram_check(const char* where) {
        const int64_t free_ram =
            (int64_t) get_platform()->available_ram_in_bytes();
        std::printf("[bench] ram @ %-14s free=%lld\n", where,
                    (long long) free_ram);
        std::fflush(stdout);
    }

    void print_scene_report() {
        unsigned total_vertices = 0;
        mesh_count_ = 0;
        assets->each_mesh([&](uint32_t, MeshPtr mesh) {
            mesh_count_++;
            total_vertices += mesh->vertex_data->count();
        });
        actor_count_ = unsigned(world_->count_nodes_by_type(Actor::Meta::node_type));

        std::printf(
            "[bench] scene: %dx%d maze, level walls=%u floor=%u "
            "(static total=%u), meshes=%u registered_vertices=%u\n",
            MAZE_W, MAZE_H, wall_vertices_, floor_vertices_,
            wall_vertices_ + floor_vertices_, mesh_count_, total_vertices);
        std::printf(
            "[bench] dynamic: drones=%u sentinels=%u torches=%u "
            "props=%u orbs=%u sounds=%u\n",
            (unsigned) drones_.size(), (unsigned) sentinels_.size(),
            (unsigned) fire_systems_.size(), 70u + 24u, (unsigned) orbs_.size(),
            (unsigned) sfx_sources_.size());
        std::printf(
            "[bench] camera path: %u cells, %u curve points, %.1f units\n",
            (unsigned) path_.size(), (unsigned) cam_points_.size(),
            double(cam_length_));
        std::fflush(stdout);
    }

    void print_summary() {
        std::vector<float> sorted = frame_ms_;
        std::sort(sorted.begin(), sorted.end());

        auto pct = [&](double p) -> float {
            if(sorted.empty()) {
                return 0.0f;
            }
            std::size_t idx =
                std::size_t(p * double(sorted.size() - 1) + 0.5);
            idx = std::min(idx, sorted.size() - 1);
            return sorted[idx];
        };

        double sum = 0.0;
        for(float m: sorted) {
            sum += double(m);
        }
        const double avg = sorted.empty() ? 0.0 : sum / double(sorted.size());

        std::printf("\n==== SECTOR BENCHMARK SUMMARY ====\n");
        std::printf("requested duration : %.1f s\n", double(RUN_SECONDS));
        std::printf("measured duration   : %.3f s\n", double(run_time_));
        std::printf("frames             : %u\n", (unsigned) sorted.size());
        std::printf("avg fps            : %.2f\n",
                    avg > 0.0 ? 1000.0 / avg : 0.0);
        std::printf("frame ms  avg/min/max: %.3f / %.3f / %.3f\n", avg,
                    sorted.empty() ? 0.0 : double(sorted.front()),
                    sorted.empty() ? 0.0 : double(sorted.back()));
        std::printf("frame ms  p50/p95/p99: %.3f / %.3f / %.3f\n",
                    double(pct(0.50)), double(pct(0.95)), double(pct(0.99)));
        std::printf("level vertices       : %u (walls %u, floor %u)\n",
                    wall_vertices_ + floor_vertices_, wall_vertices_,
                    floor_vertices_);
        std::printf("explosions / sounds  : %u / %u\n", explosion_count_,
                    sound_count_);
        std::printf("meshes / actors      : %u / %u\n", mesh_count_,
                    actor_count_);
        std::printf("=====================================\n\n");
        std::fflush(stdout);
    }

    /* Optional debug helper: set SIMULANT_BENCH_CAPTURE=<file.ppm> to dump frames a
     * few seconds in (numbered _1.._3 before the extension). The PPM is
     * bottom-up RGBA converted to RGB. */
    void maybe_capture() {
        if(capture_path_.empty() || capture_index_ >= capture_times_count_ ||
           run_time_ < capture_times_[capture_index_] ||
           frame_ms_.size() < 60) {
            return;
        }

        const uint32_t w = window->width();
        const uint32_t h = window->height();
        std::vector<uint8_t> rgba(std::size_t(w) * h * 4);
        if(!window->renderer->read_pixels(0, 0, w, h, rgba.data())) {
            std::printf("[bench] frame capture not supported by renderer\n");
            capture_index_ = capture_times_count_;
            return;
        }

        char path[512];
        const std::size_t dot = capture_path_.find_last_of('.');
        if(dot == std::string::npos) {
            snprintf(path, sizeof(path), "%s_%d.ppm",
                          capture_path_.c_str(), capture_index_ + 1);
        } else {
            snprintf(path, sizeof(path), "%s_%d%s",
                          capture_path_.substr(0, dot).c_str(),
                          capture_index_ + 1,
                          capture_path_.substr(dot).c_str());
        }

        FILE* fp = std::fopen(path, "wb");
        if(!fp) {
            return;
        }
        std::fprintf(fp, "P6\n%u %u\n255\n", w, h);
        std::vector<uint8_t> row(std::size_t(w) * 3);
        for(int y = int(h) - 1; y >= 0; --y) {
            for(uint32_t x = 0; x < w; ++x) {
                const uint8_t* p = &rgba[(std::size_t(y) * w + x) * 4];
                row[x * 3 + 0] = p[0];
                row[x * 3 + 1] = p[1];
                row[x * 3 + 2] = p[2];
            }
            std::fwrite(row.data(), 1, row.size(), fp);
        }
        std::fclose(fp);
        std::printf("[bench] captured frame to %s\n", path);
        capture_index_++;
    }

    void write_csv() {
#ifndef __DREAMCAST__
        std::ofstream out("benchmark_frames.csv");
        if(!out) {
            return;
        }
        out << "frame,ms\n";
        for(std::size_t i = 0; i < frame_ms_.size(); ++i) {
            out << i << "," << frame_ms_[i] << "\n";
        }
        std::printf("[bench] wrote benchmark_frames.csv (%u frames)\n",
                    (unsigned) frame_ms_.size());
#endif
    }

    void finish() {
        if(finished_) {
            return;
        }
        finished_ = true;

        print_second_line();
        if(get_app()->profiling_enabled()) {
            S_PROFILE_DUMP_TO_STDOUT();
        }
        print_summary();
        write_csv();

        std::printf("[bench] exit\n");
        std::fflush(stdout);
        get_app()->stop_running();
    }

    /* ------------------------------------------------------------------ */
    /* State                                                              */
    /* ------------------------------------------------------------------ */

    Stage* world_ = nullptr;
    Stage* hud_ = nullptr;
    FrustumCuller* level_culler_ = nullptr;

    CameraPtr camera_ = nullptr;
    CameraPtr panel_cam_ = nullptr;
    StatsPanel* stats_panel_ = nullptr;
    ui::Label* hud_label_ = nullptr;
    ui::ProgressBar* hud_bar_ = nullptr;

    DirectionalLight* sun_ = nullptr;
    PointLight* headlamp_ = nullptr;
    ShadowCaster* shadows_ = nullptr;

    TexturePtr wall_tex_;
    TexturePtr floor_tex_;
    TexturePtr crate_tex_;

    ParticleScriptPtr fire_script_;
    ParticleScriptPtr explosion_script_;
    ParticleScriptPtr trail_script_;

    SoundPtr ambient_sound_;
    SoundPtr explosion_sound_;
    PrefabPtr cube_prefab_;
    PrefabPtr rig_prefab_;

    MeshPtr crate_mesh_;
    MeshPtr barrel_mesh_;
    MeshPtr orb_mesh_;

    std::vector<uint8_t> walls_;
    std::vector<int> path_;
    std::vector<Vec3> cam_points_;
    std::vector<Vec3> cam_tangents_;
    std::vector<float> cam_arc_;
    float cam_length_ = 0.0f;

    std::vector<PrefabInstance*> drones_;
    std::vector<float> drone_phase_;
    std::vector<PrefabInstance*> sentinels_;
    std::vector<Actor*> orbs_;
    std::vector<ParticleSystem*> fire_systems_;
    std::vector<ParticleSystem*> trail_systems_;
    std::vector<PointLight*> torch_lights_;
    std::vector<float> torch_base_intensity_;
    std::vector<AudioSource*> sfx_sources_;

    AudioSource* ambient_source_ = nullptr;
    PlayingSoundPtr ambient_playing_;

    std::mt19937 rng_;

    bool ram_reported_first_frame_ = false;
    bool shot_taken_ = false;

    std::string capture_path_;
    static constexpr int capture_times_count_ = 3;
    const float capture_times_[capture_times_count_] = {4.0f, 8.0f, 12.0f};
    int capture_index_ = 0;

    float walk_distance_ = 0.0f;
    float walk_phase_ = 0.0f;
    float walk_time_ = 0.0f;
    float torch_time_ = 0.0f;
    float explosion_timer_ = EXPLOSION_INTERVAL * 0.5f;
    std::size_t sfx_index_ = 0;

    uint64_t last_frame_us_ = 0;
    uint64_t frame_counter_ = 0;
    float run_time_ = 0.0f;
    float second_accum_ = 0.0f;
    bool finished_ = false;

    std::vector<float> frame_ms_;
    std::vector<float> seconds_frames_;
    std::vector<float> seconds_polys_;
    std::vector<float> seconds_renderables_;

    uint32_t wall_vertices_ = 0;
    uint32_t floor_vertices_ = 0;
    unsigned mesh_count_ = 0;
    unsigned actor_count_ = 0;
    unsigned explosion_count_ = 0;
    unsigned sound_count_ = 0;
};

class BenchmarkApp: public smlt::Application {
public:
    BenchmarkApp(const AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() override {
        scenes->register_scene<BenchmarkScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "SECTOR Benchmark";
    config.fullscreen = false;
    config.target_frame_rate = 0; /* uncapped - this is a benchmark */
    /* On Dreamcast this starts the KOS sampling profiler (gmon.out); on
     * desktop it uncaps and dumps the engine's own profile sections. */
    config.development.force_profiling = true;
#ifdef __DREAMCAST__
    /* Keep the serial console quiet: every log line is a blocking write over
     * the dcload link and badly distorts timing (and the sampling profile). */
    config.log_level = LOG_LEVEL_ERROR;
    /* Bypass ALdc/OpenAL on Dreamcast: the sound driver's thread has been
     * implicated in bring-up hangs. */
    config.development.force_sound_driver = "null";
#else
    config.log_level = LOG_LEVEL_INFO;
#endif
    config.general.stage_node_pool_size = 2048;

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 720;
#endif

    BenchmarkApp app(config);
    return app.run();
}
