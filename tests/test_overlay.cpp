#include <unittest++/UnitTest++.h>

#include <vector>

#include "kglt/shortcuts.h"
#include "kglt/kglt.h"
#include "kglt/object.h"

using namespace kglt;

TEST(test_overlay_creation) {
    kglt::Window window;
    kglt::Scene& scene = window.scene();

    OverlayID oid = scene.new_overlay();
    CHECK(oid > 0);

    //Get a handle to the overlay
    Overlay& overlay = scene.overlay(oid);

    //Generate a mesh
    kglt::Mesh& mesh = kglt::return_new_mesh(scene);

    //Shouldn't be able to set the parent of an overlay to a mesh
    CHECK_THROW(overlay.set_parent(&mesh), generic::InvalidParentNodeError);

    overlay.set_parent(&scene); //Should work

}

