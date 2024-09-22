
#include "game.h"

void GameScene::on_load() {
    // Build your scene here
    auto scene = load_tree("assets/scene.gltf");

    compositor->create_layer(
        scene, (smlt::Camera*)scene->find_descendent_with_name("Camera"));
}

void GameScene::on_update(float dt) {
    // Update your scene here, called every frame
}

void GameScene::on_activate() {
    // Called when the scene is made active (after load)
}

void GameScene::on_deactivate() {
    // Called before unloading
}

void GameScene::on_unload() {
    // Cleanup your scene here
}
