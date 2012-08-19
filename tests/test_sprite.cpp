#include <unittest++/UnitTest++.h>

#include <vector>

#include "kglt/shortcuts.h"
#include "kglt/kglt.h"
#include "kglt/object.h"

using namespace kglt;

TEST(test_deleting_sprites_deletes_children) {
    kglt::Window window;
    kglt::Scene& scene = window.scene();

    kglt::SpriteID oid = scene.new_sprite(); //Create the sprite
    kglt::MeshID cid1 = scene.new_mesh(&scene.sprite(oid)); //Create a child
    kglt::MeshID cid2 = scene.new_mesh(&scene.mesh(cid1)); //Crete a child of the child

    CHECK_EQUAL(2, scene.sprite(oid).child_count()); //Sprites already have a child
    CHECK_EQUAL(1, scene.mesh(cid1).child_count());
    CHECK_EQUAL(0, scene.mesh(cid2).child_count());

    scene.delete_sprite(oid);

    CHECK(!scene.has_sprite(oid));
    CHECK(!scene.has_mesh(cid1));
    CHECK(!scene.has_mesh(cid2));
}

