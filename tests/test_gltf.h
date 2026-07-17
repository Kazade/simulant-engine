#pragma once

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
};

} // namespace
