#ifndef SKYBOX_H
#define SKYBOX_H

#include "../types.h"
#include "../generic/managed.h"
#include "../generic/auto_weakptr.h"

namespace kglt{

class Scene;

namespace extra {

class SkyBox :
    public Managed<SkyBox> {

public:
    SkyBox(AutoWeakPtr<Stage> stage, TextureID texture, float size=500.0f, CameraID cam=CameraID());
    SkyBox(AutoWeakPtr<Stage> stage, TextureID front, TextureID back, TextureID left, TextureID right, TextureID top, TextureID bottom);

private:
    AutoWeakPtr<Stage> stage_;

    MaterialID material_id_;
    ActorID actor_;

    CameraID camera_id_;
};

class StarField :
    public Managed<StarField> {

public:
    StarField(AutoWeakPtr<Stage> stage, CameraID cam=CameraID());

private:
    SkyBox::ptr skybox_;
    TextureID texture_id_;
};

}
}

#endif // SKYBOX_H
