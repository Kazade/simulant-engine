
#include "md2_loader.h"
#include "../mesh.h"
#include "../resource_manager.h"
#include "../resource_locator.h"


namespace kglt {
namespace loaders{

struct MD2Header {
    int32_t ident;                  /* magic number: "IDP2" */
    int32_t version;                /* version: must be 8 */

    int32_t skinwidth;              /* texture width */
    int32_t skinheight;             /* texture height */

    int32_t framesize;              /* size in bytes of a frame */

    int32_t num_skins;              /* number of skins */
    int32_t num_vertices;           /* number of vertices per frame */
    int32_t num_st;                 /* number of texture coordinates */
    int32_t num_tris;               /* number of triangles */
    int32_t num_glcmds;             /* number of opengl commands */
    int32_t num_frames;             /* number of frames */

    int32_t offset_skins;           /* offset skin data */
    int32_t offset_st;              /* offset texture coordinate data */
    int32_t offset_tris;            /* offset triangle data */
    int32_t offset_frames;          /* offset frame data */
    int32_t offset_glcmds;          /* offset OpenGL command data */
    int32_t offset_end;             /* offset end of file */
};

struct MD2Skin {
    char name[64];
};

struct MD2TexCoord {
    int16_t s;
    int16_t t;
};

struct MD2Triangle {
    uint16_t index[3];
    uint16_t st[3];
};

struct MD2Vertex {
    uint8_t v[3];
    uint8_t normal;
};

const int32_t MAGIC_NUMBER_ID = 844121161;

void MD2Loader::into(Loadable &resource, const LoaderOptions &options) {
    Mesh* mesh = loadable_to<Mesh>(resource);
    ResourceManager* resource_manager = &mesh->resource_manager();

    assert(mesh && "Tried to load an MD2 file into something that wasn't a mesh");

    std::string parent_dir = kfs::path::dir_name(filename_.encode());
    std::string data = this->data_->str();

    // Cast the raw header data
    MD2Header* header = (MD2Header*) &data[0];

    if(header->ident != MAGIC_NUMBER_ID || header->version != 8) {
        throw std::logic_error("Unsupported MD2 file: " + this->filename_.encode());
    }

    auto get_frame_vertex_position = [header, &data](int32_t frame_index, int16_t vertex_index) -> Vec3 {
        //Needed because the Quake 2 coord system is weird
        kmMat4 rotation;
        kmMat4RotationX(&rotation, kmDegreesToRadians(-90.0f));

        const char* cursor = &data[0];
        cursor += header->offset_frames;
        cursor += header->framesize * frame_index;

        Vec3* scale = (Vec3*) cursor;
        cursor += sizeof(float) * 3;

        Vec3* translate = (Vec3*) cursor;
        cursor += sizeof(float) * 3;
        cursor += sizeof(char) * 16;

        cursor += (sizeof(MD2Vertex) * vertex_index);

        MD2Vertex* vert = (MD2Vertex*) cursor;

        float x = float(vert->v[0]) * scale->x + translate->x;
        float y = float(vert->v[1]) * scale->y + translate->y;
        float z = float(vert->v[2]) * scale->z + translate->z;
        auto ret = Vec3(x, y, z);
        kmVec3Transform(&ret, &ret, &rotation);
        return ret;
    };


    MaterialID material;
    std::vector<Vec2> texture_coordinates;

    char* cursor = &data[0] + sizeof(MD2Header);
    for(int32_t i = 0; i < header->num_skins; ++i) {
        MD2Skin* skin = (MD2Skin*) cursor;

        if(i == 0) {
            // Only load the first skin

            kglt::TextureID tex_id;
            try {
                std::string texture_path = "sample_data/ogro.png";
                tex_id = resource_manager->new_texture_from_file(locator->locate_file(texture_path));
            } catch(ResourceMissingError&) {
                L_WARN("Unable to locate MD2 skin: " + std::string((char*) skin->name));
                tex_id = resource_manager->default_texture_id();
            }

            material = resource_manager->new_material_from_texture(tex_id);
        }
        cursor += sizeof(MD2Skin);
    }

    for(int32_t i = 0; i < header->num_st; ++i) {
        MD2TexCoord* coord = (MD2TexCoord*) cursor;
        texture_coordinates.push_back(
            Vec2(
                float(coord->s) / float(header->skinwidth),
                -float(coord->t) / float(header->skinheight)
            )
        );

        cursor += sizeof(MD2TexCoord);
    }

    VertexSpecification vertex_specification;
    vertex_specification.position_attribute = VERTEX_ATTRIBUTE_3F;
    vertex_specification.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    vertex_specification.normal_attribute = VERTEX_ATTRIBUTE_3F;
    vertex_specification.diffuse_attribute = VERTEX_ATTRIBUTE_4F;

    // Rebuild the mesh from the loaded data
    mesh->reset(vertex_specification);
    mesh->enable_animation(MESH_ANIMATION_TYPE_VERTEX_MORPH, header->num_frames);

    SubMesh* submesh = mesh->new_submesh(
        "default",
        MESH_ARRANGEMENT_TRIANGLES,
        VERTEX_SHARING_MODE_SHARED
    );

    submesh->set_material_id(material);

    /* Each index in an MD2 triangle may have its own texture coordinate. As
     * we store texture coords on the vertex data, we need to duplicate the position
     * and normal data when this happens. We create a key representing a combination of
     * a vertex index and a texture coordinate index and only push new vertices of that
     * combination hasn't been seen before
     */
    typedef std::pair<uint16_t, uint16_t> VertexKey;

    // Lookup to the index in our vertex data
    std::map<VertexKey, uint32_t> vertex_lookup;

    char* triangle_cursor = cursor; // Store for later

    for(int32_t i = 0; i < header->num_tris; ++i) {
        MD2Triangle* tri = (MD2Triangle*) cursor;

        for(int32_t j = 0; j < 3; ++j) {
            VertexKey key = std::make_pair(tri->index[j], tri->st[j]);

            if(vertex_lookup.count(key)) {
                submesh->index_data->index(vertex_lookup.at(key));
            } else {
                // This just populates the first frame, after that point we know how many
                // vertices we need per frame and we can populate the other frames
                Vec3 v = get_frame_vertex_position(0, tri->index[j]);
                submesh->vertex_data->position(v);
                submesh->vertex_data->diffuse(kglt::Colour::WHITE);
                submesh->vertex_data->tex_coord0(texture_coordinates[tri->st[j]]);
                submesh->vertex_data->normal(Vec3(0, 1, 0));
                submesh->vertex_data->move_next();

                vertex_lookup[key] = submesh->vertex_data->count() - 1;
                submesh->index_data->index(vertex_lookup.at(key));
            }
        }

        cursor += sizeof(MD2Triangle);
    }

    // This is necessary due to the difference in coordinate system
    submesh->reverse_winding();

    // Make room for all of the frame vertices
    uint32_t vertices_per_frame = submesh->vertex_data->count();
    submesh->vertex_data->resize(vertices_per_frame * header->num_frames);

    // Go through populating the same index as the first frame plus the frame offset
    // with the combined position + texcoord data
    for(int32_t frame_id = 0; frame_id < header->num_frames; ++frame_id) {
        for(int32_t i = 0; i < header->num_tris; ++i) {
            MD2Triangle* tri = ((MD2Triangle*) triangle_cursor) + i;

            for(int32_t j = 0; j < 3; ++j) {
                VertexKey key = std::make_pair(tri->index[j], tri->st[j]);

                Vec3 v = get_frame_vertex_position(frame_id, tri->index[j]);
                submesh->vertex_data->move_to((frame_id * vertices_per_frame) + vertex_lookup[key]);
                submesh->vertex_data->position(v);
                submesh->vertex_data->diffuse(kglt::Colour::WHITE);
                submesh->vertex_data->tex_coord0(texture_coordinates[tri->st[j]]);
                submesh->vertex_data->normal(Vec3(0, 1, 0));
            }
        }
    }

    submesh->index_data->done();
    submesh->vertex_data->done();

    mesh->add_animation("test", 0, 8, 10.0);
}


} //loaders
} //kglt
