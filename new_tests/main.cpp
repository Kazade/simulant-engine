

#include <tr1/functional>
#include <tr1/memory>
#include <kglt/kazbase/testing.h>

#include "/home/kazade/Git/KGLT/new_tests/test_frustum.h"
#include "/home/kazade/Git/KGLT/new_tests/test_vertex_data.h"
#include "/home/kazade/Git/KGLT/new_tests/test_mesh.h"
#include "/home/kazade/Git/KGLT/new_tests/test_octree.h"
#include "/home/kazade/Git/KGLT/new_tests/test_material_script.h"
#include "/home/kazade/Git/KGLT/new_tests/test_sound.h"
#include "/home/kazade/Git/KGLT/new_tests/test_shader.h"
#include "/home/kazade/Git/KGLT/new_tests/test_material.h"
#include "/home/kazade/Git/KGLT/new_tests/test_camera.h"
#include "/home/kazade/Git/KGLT/new_tests/global.h"

int main(int argc, char* argv[]) {
    std::tr1::shared_ptr<TestRunner> runner(new TestRunner());
    
    
    runner->register_case<FrustumTest>(
        std::vector<void (FrustumTest::*)()>({&FrustumTest::test_frustum_generation}), 
        {"FrustumTest::test_frustum_generation"}
    );


    runner->register_case<CameraTest>(
        std::vector<void (CameraTest::*)()>({&CameraTest::test_project_point}), 
        {"CameraTest::test_project_point"}
    );


    runner->register_case<ShaderTest>(
        std::vector<void (ShaderTest::*)()>({&ShaderTest::test_shader_params}), 
        {"ShaderTest::test_shader_params"}
    );


    runner->register_case<SoundTest>(
        std::vector<void (SoundTest::*)()>({&SoundTest::test_2d_sound_output, &SoundTest::test_3d_sound_output}), 
        {"SoundTest::test_2d_sound_output", "SoundTest::test_3d_sound_output"}
    );


    runner->register_case<MeshTest>(
        std::vector<void (MeshTest::*)()>({&MeshTest::test_user_data_works, &MeshTest::test_deleting_entities_deletes_children, &MeshTest::test_procedural_rectangle_outline, &MeshTest::test_basic_usage, &MeshTest::test_entity_from_mesh, &MeshTest::test_scene_methods}), 
        {"MeshTest::test_user_data_works", "MeshTest::test_deleting_entities_deletes_children", "MeshTest::test_procedural_rectangle_outline", "MeshTest::test_basic_usage", "MeshTest::test_entity_from_mesh", "MeshTest::test_scene_methods"}
    );


    runner->register_case<OctreeTest>(
        std::vector<void (OctreeTest::*)()>({&OctreeTest::test_moving_objects, &OctreeTest::test_insertion}), 
        {"OctreeTest::test_moving_objects", "OctreeTest::test_insertion"}
    );


    runner->register_case<MaterialTest>(
        std::vector<void (MaterialTest::*)()>({&MaterialTest::test_material_initialization, &MaterialTest::test_material_applies_to_mesh}), 
        {"MaterialTest::test_material_initialization", "MaterialTest::test_material_applies_to_mesh"}
    );


    runner->register_case<MaterialScriptTest>(
        std::vector<void (MaterialScriptTest::*)()>({&MaterialScriptTest::test_basic_material_script_parsing}), 
        {"MaterialScriptTest::test_basic_material_script_parsing"}
    );


    runner->register_case<VertexDataTest>(
        std::vector<void (VertexDataTest::*)()>({&VertexDataTest::test_offsets}), 
        {"VertexDataTest::test_offsets"}
    );

    
    return runner->run();
}


