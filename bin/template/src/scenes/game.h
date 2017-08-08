#pragma once

#include <simulant/simulant.h>

class GameScene : public smlt::Scene<GameScene> {
public:
    // Boilerplate
    GameScene(smlt::WindowBase& window):
        smlt::Scene<GameScene>(window) {}

    void load();
    void update(float dt);
    void activate();
    void deactivate();
    void unload();
};
