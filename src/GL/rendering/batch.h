#ifndef KGLT_BATCH_H
#define KGLT_BATCH_H

namespace GL {
namespace rendering {

enum VertexInfoFlag {
    POSITION,
    COLOUR,
    NORMAL,
    SECONDARY_COLOUR,
    TEXTURE_COORD,
    SECONDARY_TEXTURE_COORD
};

class VertexInfo {
    uint32_t flags;

    float pos[3];
    float colour[4];
    float normal[3];
    float secondary_colour[4];
    float texture_coord[2];
    float secondary_texture_coord[2];
};

class Group {
public:
    virtual void set_state() = 0;
    virtual void unset_state() = 0;
};

class NullGroup : public Group {
public:
    void set_state() {};
    void unset_state() {};
}

class Batch {
public:
    void add(uint32_t count, GLenum mode, const Group& group, std::vector<VertexInfo> data);
};

/*
   Batch batch;
   batch.add(3, GL_TRIANGLES, NullGroup(), data);
   batch.add(3, GL_TRIANGLES, TextureBindGroup(tex_id), data);
   batch.add(3, GL_TRIANGLES, ShaderGroup(shader_id, parent=TextureBindGroup(tex_id)), data);
   batch.add(3, GL_TRIANGLES, TextureBindGroup(other_tex_id), data);
   batch.draw()

   //Drawing order:
   #Draw NullGroup stuff
   #Draw TextureBindGroup stuff wih tex_id
   #Draw ShaderGroup stuff with tex_id and shader_id
   #Draw TextureBindGroup stuff with other_tex_id

   Tree:
   
   NullGroup -   TextureBindGroup(tex_id) - TextureBindGroup(other_tex_id)
                          |
                   ShaderGroup(shader_id)

  

*/


}
}

#endif
