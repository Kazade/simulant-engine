#pragma once

#include <simulant/nodes/animation_controller.h>
#include <simulant/simulant.h>
#include <simulant/test.h>

namespace {

using namespace smlt;

const std::string GLTF_FILE = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],
  
  "nodes" : [
    {
      "name": "test",
      "mesh" : 0
    }
  ],
  
  "meshes" : [
    {
      "primitives" : [ {
        "attributes" : {
          "POSITION" : 1
        },
        "indices" : 0
      } ]
    }
  ],

  "buffers" : [
    {
      "uri" : "data:application/octet-stream;base64,AAABAAIAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAACAPwAAAAA=",
      "byteLength" : 44
    }
  ],
  "bufferViews" : [
    {
      "buffer" : 0,
      "byteOffset" : 0,
      "byteLength" : 6,
      "target" : 34963
    },
    {
      "buffer" : 0,
      "byteOffset" : 8,
      "byteLength" : 36,
      "target" : 34962
    }
  ],
  "accessors" : [
    {
      "bufferView" : 0,
      "byteOffset" : 0,
      "componentType" : 5123,
      "count" : 3,
      "type" : "SCALAR",
      "max" : [ 2 ],
      "min" : [ 0 ]
    },
    {
      "bufferView" : 1,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 3,
      "type" : "VEC3",
      "max" : [ 1.0, 1.0, 0.0 ],
      "min" : [ 0.0, 0.0, 0.0 ]
    }
  ],
  
  "asset" : {
    "version" : "2.0"
  }
}
)";

// A single quad (4 vertices, 2 triangles) encoded as a primitive with
// "mode": 5 (TRIANGLE_STRIP), as produced by tools/optimise_gltf.
const std::string GLTF_FILE_TRIANGLE_STRIP = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "name": "strip_test",
      "mesh" : 0
    }
  ],

  "meshes" : [
    {
      "primitives" : [ {
        "attributes" : {
          "POSITION" : 1
        },
        "indices" : 0,
        "mode": 5
      } ]
    }
  ],

  "buffers" : [
    {
      "uri" : "data:application/octet-stream;base64,AAABAAIAAwAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAACAPwAAAAAAAIA/AACAPwAAAAA=",
      "byteLength" : 56
    }
  ],
  "bufferViews" : [
    {
      "buffer" : 0,
      "byteOffset" : 0,
      "byteLength" : 8,
      "target" : 34963
    },
    {
      "buffer" : 0,
      "byteOffset" : 8,
      "byteLength" : 48,
      "target" : 34962
    }
  ],
  "accessors" : [
    {
      "bufferView" : 0,
      "byteOffset" : 0,
      "componentType" : 5123,
      "count" : 4,
      "type" : "SCALAR",
      "max" : [ 3 ],
      "min" : [ 0 ]
    },
    {
      "bufferView" : 1,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 4,
      "type" : "VEC3",
      "max" : [ 1.0, 1.0, 0.0 ],
      "min" : [ 0.0, 0.0, 0.0 ]
    }
  ],

  "asset" : {
    "version" : "2.0"
  }
}
)";

// glTF 2.1 "Shapes": a node with no mesh of its own, carrying a
// boundingVolume that references the top-level shapes array.
// glTF 2.1 "Non-Sequential Attributes": the primitive below declares only
// "TEXCOORD_1" and "COLOR_1" (no "TEXCOORD_0"/"COLOR_0" at all), which
// glTF 2.0 forbade (attribute sets had to start at 0 and be contiguous).
const std::string GLTF_FILE_NON_SEQUENTIAL_ATTRIBUTES = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "name": "test",
      "mesh" : 0
    }
  ],

  "meshes" : [
    {
      "primitives" : [ {
        "attributes" : {
          "POSITION" : 1,
          "TEXCOORD_1" : 2,
          "COLOR_1" : 3
        },
        "indices" : 0
      } ]
    }
  ],

  "buffers" : [
    {
      "uri" : "data:application/octet-stream;base64,AAABAAIAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgD8=",
      "byteLength" : 116
    }
  ],
  "bufferViews" : [
    {
      "buffer" : 0,
      "byteOffset" : 0,
      "byteLength" : 6,
      "target" : 34963
    },
    {
      "buffer" : 0,
      "byteOffset" : 8,
      "byteLength" : 36,
      "target" : 34962
    },
    {
      "buffer" : 0,
      "byteOffset" : 44,
      "byteLength" : 24,
      "target" : 34962
    },
    {
      "buffer" : 0,
      "byteOffset" : 68,
      "byteLength" : 48,
      "target" : 34962
    }
  ],
  "accessors" : [
    {
      "bufferView" : 0,
      "byteOffset" : 0,
      "componentType" : 5123,
      "count" : 3,
      "type" : "SCALAR",
      "max" : [ 2 ],
      "min" : [ 0 ]
    },
    {
      "bufferView" : 1,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 3,
      "type" : "VEC3",
      "max" : [ 1.0, 1.0, 0.0 ],
      "min" : [ 0.0, 0.0, 0.0 ]
    },
    {
      "bufferView" : 2,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 3,
      "type" : "VEC2"
    },
    {
      "bufferView" : 3,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 3,
      "type" : "VEC4"
    }
  ],

  "asset" : {
    "version" : "2.1"
  }
}
)";

// glTF 2.1 "Accessor Component Type Definitions": the animation output
// accessor below is stored as HALF_FLOAT (componentType 5131), one of the
// new component types introduced in 2.1. It should still decode correctly
// into a translation keyframe even though it's not FLOAT.
const std::string GLTF_FILE_HALF_FLOAT_ANIMATION = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "name": "anim_node"
    }
  ],

  "animations" : [
    {
      "name": "anim",
      "channels" : [
        { "sampler" : 0, "target" : { "node" : 0, "path" : "translation" } }
      ],
      "samplers" : [
        { "input" : 0, "output" : 1, "interpolation" : "LINEAR" }
      ]
    }
  ],

  "accessors" : [
    {
      "bufferView" : 0,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 2,
      "type" : "SCALAR"
    },
    {
      "bufferView" : 1,
      "byteOffset" : 0,
      "componentType" : 5131,
      "count" : 2,
      "type" : "VEC3"
    }
  ],
  "bufferViews" : [
    {
      "buffer" : 0,
      "byteOffset" : 0,
      "byteLength" : 8
    },
    {
      "buffer" : 0,
      "byteOffset" : 8,
      "byteLength" : 12
    }
  ],
  "buffers" : [
    {
      "uri" : "data:application/octet-stream;base64,AAAAAAAAgD8AAAAAAAAAPABAAEI=",
      "byteLength" : 20
    }
  ],

  "asset" : {
    "version" : "2.1"
  }
}
)";

const std::string GLTF_FILE_SHAPE = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "name": "test",
      "boundingVolume": { "shape": 0 }
    }
  ],

  "shapes": [
    { "type": "box", "box": { "size": [2.0, 3.0, 4.0] } }
  ],

  "asset" : {
    "version" : "2.1"
  }
}
)";

// glTF 2.1 "External Assets": the child file instantiated by
// GLTF_FILE_EXTERNAL_PARENT below via its top-level "files" array and a
// node "asset" reference.
const std::string GLTF_FILE_EXTERNAL_CHILD = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "name": "child_node"
    }
  ],

  "asset" : {
    "version" : "2.1"
  }
}
)";

const std::string GLTF_FILE_EXTERNAL_PARENT = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "name": "asset_ref",
      "asset": 0
    }
  ],

  "files": [
    { "uri": "gltf21_external_child.gltf" }
  ],

  "asset" : {
    "version" : "2.1"
  }
}
)";

// glTF 2.1 "External Assets": a pair of files referencing each other, used
// to verify that a cyclical reference (A -> B -> A) is refused instead of
// recursing forever.
const std::string GLTF_FILE_CYCLE_A = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "name": "a_ref",
      "asset": 0
    }
  ],

  "files": [
    { "uri": "gltf21_cycle_b.gltf" }
  ],

  "asset" : {
    "version" : "2.1"
  }
}
)";

const std::string GLTF_FILE_CYCLE_B = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "name": "b_ref",
      "asset": 0
    }
  ],

  "files": [
    { "uri": "gltf21_cycle_a.gltf" }
  ],

  "asset" : {
    "version" : "2.1"
  }
}
)";

class GLTFLoaderTests: public smlt::test::SimulantTestCase {
public:
    void test_load_gltf_file() {
        auto temp_dir = Path::system_temp_dir();
        auto test_file = temp_dir.append("test.gltf");

        std::ofstream fileout(test_file.str().c_str());
        assert_true(fileout.good());
        fileout.write(GLTF_FILE.c_str(), GLTF_FILE.size());
        fileout.close();

        auto node = scene->load_tree(test_file);

        assert_true(node->child_count() > 0);
        assert_equal(node->child_at(0)->name(), "test");
    }

    // Regression test: primitive "mode" is a per-primitive glTF property
    // (mesh.primitives[i].mode), not a property of the mesh object. The
    // loader used to read it from the wrong JSON node (mesh["mode"], which
    // never exists) and so always fell back to TRIANGLES, silently
    // mis-rendering every TRIANGLE_STRIP primitive (e.g. the output of
    // tools/optimise_gltf) as if it were a plain triangle list.
    void test_load_gltf_triangle_strip_mode() {
        auto temp_dir = Path::system_temp_dir();
        auto test_file = temp_dir.append("test_strip.gltf");

        std::ofstream fileout(test_file.str().c_str());
        assert_true(fileout.good());
        fileout.write(GLTF_FILE_TRIANGLE_STRIP.c_str(),
                       GLTF_FILE_TRIANGLE_STRIP.size());
        fileout.close();

        auto node = scene->load_tree(test_file);

        auto actors =
            node->find_descendents_by_types({Actor::Meta::node_type});
        assert_equal((std::size_t)1, actors.size());

        auto actor = dynamic_cast<ActorPtr>(actors[0]);
        assert_true(actor != nullptr);

        auto sm = actor->base_mesh()->first_submesh();
        assert_true(sm != nullptr);
        assert_equal(MESH_ARRANGEMENT_TRIANGLE_STRIP, sm->arrangement());
        assert_equal(4u, sm->index_data->count());
    }

    // glTF 2.1 "Non-Sequential Attributes": a primitive whose only texcoord
    // and color sets are "TEXCOORD_1"/"COLOR_1" (no "_0" at all) must still
    // load rather than silently coming out untextured/uncoloured, even
    // though Simulant only has a single active texcoord/color channel.
    void test_load_gltf_non_sequential_attributes() {
        auto temp_dir = Path::system_temp_dir();
        auto test_file = temp_dir.append("gltf21_non_sequential.gltf");

        std::ofstream fileout(test_file.str().c_str());
        assert_true(fileout.good());
        fileout.write(GLTF_FILE_NON_SEQUENTIAL_ATTRIBUTES.c_str(),
                       GLTF_FILE_NON_SEQUENTIAL_ATTRIBUTES.size());
        fileout.close();

        auto node = scene->load_tree(test_file);

        auto actors =
            node->find_descendents_by_types({Actor::Meta::node_type});
        assert_equal((std::size_t)1, actors.size());

        auto actor = dynamic_cast<ActorPtr>(actors[0]);
        assert_true(actor != nullptr);

        auto spec = actor->base_mesh()->vertex_data->vertex_specification();
        assert_true(spec.has_texcoord0());
        assert_true(spec.has_color());

        auto sm = actor->base_mesh()->first_submesh();
        assert_true(sm != nullptr);
        assert_equal(3u, sm->index_data->count());
    }

    // glTF 2.1 "Accessor Component Type Definitions": an animation sampler
    // output accessor stored as HALF_FLOAT (one of the new componentType
    // values) should still decode into correct keyframe data.
    void test_load_gltf_half_float_animation() {
        auto temp_dir = Path::system_temp_dir();
        auto test_file = temp_dir.append("gltf21_half_float_anim.gltf");

        std::ofstream fileout(test_file.str().c_str());
        assert_true(fileout.good());
        fileout.write(GLTF_FILE_HALF_FLOAT_ANIMATION.c_str(),
                       GLTF_FILE_HALF_FLOAT_ANIMATION.size());
        fileout.close();

        auto node = scene->load_tree(test_file);

        auto controller = node->find_mixin<AnimationController>();
        assert_true(controller != nullptr);

        auto names = controller->animation_names();
        assert_equal((std::size_t)1, names.size());

        assert_true(node->child_count() > 0);
        auto anim_node = node->child_at(0);
        assert_equal(anim_node->name(), "anim_node");

        controller->seek(1.0f);

        auto translation = anim_node->transform->translation();
        assert_close(1.0f, translation.x, 0.01f);
        assert_close(2.0f, translation.y, 0.01f);
        assert_close(3.0f, translation.z, 0.01f);
    }

    // glTF 2.1 "Shapes": a node's boundingVolume should spawn a child
    // Actor wrapping a procedurally generated mesh for the referenced
    // shape, rather than requiring a mesh of its own.
    void test_load_gltf_shape_bounding_volume() {
        auto temp_dir = Path::system_temp_dir();
        auto test_file = temp_dir.append("gltf21_shape.gltf");

        std::ofstream fileout(test_file.str().c_str());
        assert_true(fileout.good());
        fileout.write(GLTF_FILE_SHAPE.c_str(), GLTF_FILE_SHAPE.size());
        fileout.close();

        auto node = scene->load_tree(test_file);

        assert_true(node->child_count() > 0);
        auto test_node = node->child_at(0);
        assert_equal(test_node->name(), "test");

        auto actors =
            test_node->find_descendents_by_types({Actor::Meta::node_type});
        assert_equal((std::size_t)1, actors.size());

        auto actor = dynamic_cast<ActorPtr>(actors[0]);
        assert_true(actor != nullptr);

        auto sm = actor->base_mesh()->find_submesh("shape");
        assert_true(sm != nullptr);
        assert_equal(sm->material()->name(), "GLTFShape");

        auto dims = actor->base_mesh()->aabb().dimensions();
        assert_close(2.0f, dims.x, 0.001f);
        assert_close(3.0f, dims.y, 0.001f);
        assert_close(4.0f, dims.z, 0.001f);
    }

    // glTF 2.1 "External Assets": a node with an "asset" property should
    // instantiate the referenced glTF file (resolved through the
    // top-level "files" array) as a nested prefab_instance sub-tree.
    void test_load_gltf_external_asset() {
        auto temp_dir = Path::system_temp_dir();

        auto child_file = temp_dir.append("gltf21_external_child.gltf");
        std::ofstream child_out(child_file.str().c_str());
        assert_true(child_out.good());
        child_out.write(GLTF_FILE_EXTERNAL_CHILD.c_str(),
                        GLTF_FILE_EXTERNAL_CHILD.size());
        child_out.close();

        auto parent_file = temp_dir.append("gltf21_external_parent.gltf");
        std::ofstream parent_out(parent_file.str().c_str());
        assert_true(parent_out.good());
        parent_out.write(GLTF_FILE_EXTERNAL_PARENT.c_str(),
                         GLTF_FILE_EXTERNAL_PARENT.size());
        parent_out.close();

        auto node = scene->load_tree(parent_file);

        assert_true(node->child_count() > 0);
        auto asset_node = node->child_at(0);
        assert_equal(asset_node->name(), "asset_ref");
        assert_equal(std::string("prefab_instance"),
                     std::string(asset_node->node_type_name()));

        assert_true(asset_node->child_count() > 0);
        assert_equal(asset_node->child_at(0)->name(), "child_node");
    }

    // glTF 2.1 "External Assets": a cyclical reference (A -> B -> A) must
    // be refused rather than recursing forever. The cyclical edge should
    // fall back to an inert node instead of a prefab_instance.
    void test_load_gltf_external_asset_cycle() {
        auto temp_dir = Path::system_temp_dir();

        auto file_a = temp_dir.append("gltf21_cycle_a.gltf");
        std::ofstream a_out(file_a.str().c_str());
        assert_true(a_out.good());
        a_out.write(GLTF_FILE_CYCLE_A.c_str(), GLTF_FILE_CYCLE_A.size());
        a_out.close();

        auto file_b = temp_dir.append("gltf21_cycle_b.gltf");
        std::ofstream b_out(file_b.str().c_str());
        assert_true(b_out.good());
        b_out.write(GLTF_FILE_CYCLE_B.c_str(), GLTF_FILE_CYCLE_B.size());
        b_out.close();

        auto node = scene->load_tree(file_a);

        assert_true(node->child_count() > 0);
        auto a_ref = node->child_at(0);
        assert_equal(a_ref->name(), "a_ref");
        assert_equal(std::string("prefab_instance"),
                     std::string(a_ref->node_type_name()));

        assert_true(a_ref->child_count() > 0);
        auto b_ref = a_ref->child_at(0);
        assert_equal(b_ref->name(), "b_ref");

        // The cycle back to A must have been refused, so b_ref falls back
        // to a plain node rather than another nested prefab_instance.
        assert_equal(std::string("stage"),
                     std::string(b_ref->node_type_name()));
        assert_equal((std::size_t)0, b_ref->child_count());
    }
};

} // namespace
