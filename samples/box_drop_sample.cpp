#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/additional.h"

using namespace kglt::extra;
using namespace kglt;

class BoxDrop: public kglt::App {
public:
    BoxDrop():
        App("KGLT BoxDrop Sample") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {
        scene().enable_physics(DefaultPhysicsEngine::create());
        scene().physics_engine()->create_plane(0, 1, 0, -4);
        scene().physics_engine()->set_gravity(Vec3(0, -7.8, 0));

        texture_id_ = stage().new_texture_from_file("sample_data/sample.tga");
        mesh_ = stage().new_mesh();
        procedural::mesh::cube(stage().mesh(mesh_), 1.0);
        stage().mesh(mesh_)->set_texture_on_material(0, texture_id_);

        return true;
    }

    void do_step(double dt) {
        if(initialized()) {
            time_since_last_spawn_ += dt;

            //Spawn a new cube every second
            if(time_since_last_spawn_ >= 1.0) {

                //Make the new actor both responsive, and collidable
                ActorID new_actor = stage().new_actor(mesh_, true, true);
                {
                    auto actor = stage().actor(new_actor);

                    //Add a cube shape to the collidable
                    actor->collidable().add_box(1.0, 1.0, 1.0);
                    actor->move_to(Vec3(0, 10, -10));
                };

                time_since_last_spawn_ = 0.0;
            }
        }
    }

    void do_cleanup() {

    }

    kglt::TextureID texture_id_;
    kglt::MeshID mesh_;
    std::vector<ActorID> actors_;

    float time_since_last_spawn_ = 0.0;

};


int main(int argc, char* argv[]) {
    BoxDrop app;
    return app.run();
}


