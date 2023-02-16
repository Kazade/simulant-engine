#pragma once

#ifdef DEFINE_STAGENODEPOOL
#include "actor.h"
#include "mesh_instancer.h"
#include "camera.h"
#include "geom.h"
#include "light.h"
#include "particle_system.h"
#include "sprite.h"
#include "skies/skybox.h"
#include "ui/button.h"
#include "ui/image.h"
#include "ui/label.h"
#include "ui/progress_bar.h"
#include "ui/frame.h"
#include "ui/keyboard.h"
#include "ui/text_entry.h"
#endif

namespace smlt {

class StageNode;
class Actor;
class MeshInstancer;
class Camera;
class Geom;
class Light;
class ParticleSystem;
class Sprite;
class Skybox;

namespace ui {

class Button;
class Image;
class Label;
class ProgressBar;
class Frame;
class Keyboard;
class TextEntry;

}

template<typename Base, typename... Classes>
class Polylist;

typedef Polylist<
    StageNode,
    Actor, MeshInstancer, Camera, Geom, Light, ParticleSystem, Sprite,
    ui::Button, ui::Image, ui::Label, ui::ProgressBar, ui::Frame, ui::Keyboard, ui::TextEntry,
    Skybox
> StageNodePool;

}
