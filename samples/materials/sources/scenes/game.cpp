
#include "game.h"

void GameScene::on_load() {
    // Build your scene here
    auto scene = load_tree("assets/scene.gltf");

    compositor->create_layer(
        scene, (smlt::Camera*)scene->find_descendent_with_name("Camera"));

    lighting->set_ambient_light(smlt::Color(0.2f, 0.2f, 0.2f, 0.2f));
}

void GameScene::on_update(float dt) {
    // Update your scene here, called every frame

    auto node = scene->find_descendent_with_name("Cube");
    auto rot = node->transform->orientation();
    rot = smlt::Quaternion(0.0_deg, smlt::Deg(dt * 20), 0.0_deg) * rot;
    node->transform->set_orientation(rot);
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
