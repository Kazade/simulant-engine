#pragma once

#include <simulant/simulant.h>

class GameScene : public smlt::Scene {
public:
    // Boilerplate
    GameScene(smlt::Window* window):
        smlt::Scene(window) {}

    void on_load();
    void on_update(float dt);
    void on_activate();
    void on_deactivate();
    void on_unload();
};
