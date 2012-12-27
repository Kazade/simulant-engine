#include "interface.h"
#include "text.h"
#include "font.h"
#include "../../scene.h"
#include "../../procedural/mesh.h"
#include "../../shortcuts.h"

namespace kglt {
namespace extra {
namespace ui {

Text::Text(Font::ptr font, Vec2 position, const unicode &text):
    interface_(font->interface()),
    font_(font) {

    Scene& scene = interface_.scene();

    mesh_ = scene.new_mesh();


    kglt::Mesh& mesh = scene.mesh(mesh_);

    uint32_t x_offset = 0;

    uint16_t i = 0;
    //Go through the text, and create a new submesh for each character
    for(char32_t ch: text) {
        const CharacterInfo& info = font->info_for_char(ch);

        x_offset += info.offset_x;

        SubMeshIndex idx = kglt::procedural::mesh::rectangle(mesh, info.width, info.height, float(x_offset) + (float(info.width) / 2.0), info.offset_y + (float(info.height) / 2.0), 0.1, false);

        MaterialID mat;

        if(container::contains(materials_, info.texture)) {
            mat = materials_[info.texture];
        } else {
            mat = kglt::create_material_from_texture(scene, info.texture);
            scene.material(mat).technique().pass(0).set_blending(BLEND_ONE_ONE_MINUS_ALPHA);
            materials_[info.texture] = mat;
        }

        mesh.submesh(idx).set_material(mat);

        x_offset -= info.offset_x;
        x_offset += info.advance_x;

        if(i++ < text.length() - 1) {
            uint16_t kern_x = info.kern_x(text[i + 1]);
            x_offset += kern_x;
        }

        mesh.submesh(idx).vertex_data().move_by(-4);
        mesh.submesh(idx).vertex_data().tex_coord0(info.texture_coordinates[0].x, info.texture_coordinates[0].y);
        mesh.submesh(idx).vertex_data().diffuse(Colour::black);
        mesh.submesh(idx).vertex_data().move_next();

        mesh.submesh(idx).vertex_data().tex_coord0(info.texture_coordinates[1].x, info.texture_coordinates[1].y);
        mesh.submesh(idx).vertex_data().diffuse(Colour::black);
        mesh.submesh(idx).vertex_data().move_next();

        mesh.submesh(idx).vertex_data().tex_coord0(info.texture_coordinates[2].x, info.texture_coordinates[2].y);
        mesh.submesh(idx).vertex_data().diffuse(Colour::black);
        mesh.submesh(idx).vertex_data().move_next();

        mesh.submesh(idx).vertex_data().tex_coord0(info.texture_coordinates[3].x, info.texture_coordinates[3].y);
        mesh.submesh(idx).vertex_data().diffuse(Colour::black);
        mesh.submesh(idx).vertex_data().move_next();
        mesh.submesh(idx).vertex_data().done();
    }

    entity_ = scene.new_entity();
    scene.entity(entity_).set_mesh(mesh_);
    scene.entity(entity_).set_position(Vec3(position.x, position.y, 0));
    scene.entity(entity_).scene_group = scene.scene_group(interface_.scene_group_id());
}

void Text::set_position(Vec2 position) {
    interface_.scene().entity(entity_).set_position(Vec3(position, 0));
}

void Text::set_colour(const Colour& colour) {
    Scene& scene = interface_.scene();
    kglt::Mesh& mesh = scene.mesh(mesh_);

    VertexData& data = mesh.submesh(mesh.submesh_ids()[0]).vertex_data();

    data.move_to_start();
    for(uint16_t i = 0; i < data.count(); ++i) {
        data.diffuse(colour);
        data.move_next();
    }
    data.move_to_end();
    data.done();
}

Text::~Text() {

}

}
}
}
