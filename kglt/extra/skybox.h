#ifndef SKYBOX_H
#define SKYBOX_H

#include "../types.h"
#include "../generic/managed.h"

namespace kglt{

class Scene;

namespace extra {

class SkyBox :
    public Managed<SkyBox> {

public:
    SkyBox(Stage& stage, TextureID texture, float size=500.0f, CameraID cam=CameraID());
    SkyBox(Stage& stage, TextureID front, TextureID back, TextureID left, TextureID right, TextureID top, TextureID bottom);

private:
    Stage& stage_;

    MaterialID material_id_;
    Actor* actor_;

    CameraID camera_id_;
};

class StarField :
    public Managed<StarField> {

public:
    StarField(Stage& stage, CameraID cam=CameraID());

private:
    SkyBox::ptr skybox_;
    TextureID texture_id_;
};

}
}

#endif // SKYBOX_H
