#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/additional.h"

using namespace kglt::extra;
using namespace kglt;

class Spinner: public kglt::App {
public:
    Spinner():
        App("KGLT Spinner Sample") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {
        scene().enable_physics(DefaultPhysicsEngine::create());

        kglt::TextureID tid = stage().new_texture_from_file("sample_data/sample.tga");

        MeshID m1 = stage().new_mesh();
        procedural::mesh::cube(stage().mesh(m1), 1.0);
        stage().mesh(m1)->set_texture_on_material(0, tid);

        parent_ = stage().new_actor(m1, true);
        stage().actor(parent_)->move_to(Vec3(0, 0, -30));

        ActorID a2 = stage().new_actor(m1, true);
        ActorID a3 = stage().new_actor(m1, true);

        stage().actor(a2)->set_parent(parent_);
        stage().actor(a3)->set_parent(parent_);

        stage().actor(a2)->set_relative_position(Vec3(3, 0, 0));
        stage().actor(a3)->set_relative_position(Vec3(0, 3, 0));

        stage().actor(parent_)->body().set_linear_velocity(Vec3(0, 0, -50));
        return true;
    }

    void do_step(double dt) {
        if(initialized()) {
            stage().actor(parent_)->body().apply_angular_force_global(Vec3(0, 10, 0));
        }
    }

    void do_cleanup() {

    }


    ActorID parent_;

};


int main(int argc, char* argv[]) {
    Spinner app;
    return app.run();
}

