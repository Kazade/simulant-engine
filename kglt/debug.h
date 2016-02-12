#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"
#include "generic/managed.h"
#include "mesh.h"

namespace kglt {

class Debug : public Managed<Debug> {
public:
    Debug(Stage& stage);
    ~Debug();

    void draw_ray(
        const Vec3& start,
        const Vec3& dir,
        const Colour& colour=Colour::WHITE,
        double duration=0.0,
        bool depth_test=true
    );


    void draw_line(
        const Vec3& start,
        const Vec3& end,
        const Colour& colour=Colour::WHITE,
        double duration=0.0,
        bool depth_test=true
    );

    void draw_point(
        const Vec3& position,
        const Colour& colour=Colour::WHITE,
        double duration=0.0,
        bool depth_test=true
    );

    bool init() override;
private:
    void update();

    Stage& stage_;

    enum DebugElementType {
        DET_LINE,
        DET_POINT
    };

    struct DebugElement {
        double time_since_created = 0.0;
        DebugElementType type = DET_LINE;
        Colour colour = Colour::WHITE;
        bool depth_test = true;
        double duration = 0.0;
        SubMeshID submesh;
    };

    std::vector<DebugElement> elements_;

    MeshID mesh_;
    ActorID actor_;
    MaterialID material_;

    sig::Connection update_connection_;
};

}

#endif // DEBUG_H
