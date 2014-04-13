#ifndef SKYBOX_H
#define SKYBOX_H

#include "../types.h"
#include "../generic/managed.h"
#include "../generic/auto_weakptr.h"

namespace kglt{

namespace extra {

class SkyBox :
    public Managed<SkyBox> {

public:
    SkyBox(StagePtr stage, TextureID texture, float size=500.0f, CameraID cam=CameraID());
    SkyBox(StagePtr stage, TextureID front, TextureID back, TextureID left, TextureID right, TextureID top, TextureID bottom);

private:
    StagePtr stage_;

    MaterialID material_id_;
    ActorID actor_;

    CameraID camera_id_;
};

class StarField :
    public Managed<StarField> {

public:
    StarField(StagePtr stage, CameraID cam=CameraID());

private:
    SkyBox::ptr skybox_;
    TextureID texture_id_;
};

}
}

#endif // SKYBOX_H
