#ifndef BATCHER_H_INCLUDED
#define BATCHER_H_INCLUDED

namespace GL {

/*
    Batcher batch;

    // Create a group with a transform, that has texture 2 bound as the secondary texture
    // and texture 1 bound as the primary texture
    TransformGroup group(matrix, TextureBindGroup(SECONDARY, 2, TextureBindGroup(PRIMARY, 1));
    batch.add(3, GL_TRIANGLES, TransformGroup, data);
    batch.add(3, GL_TRIANGLES, TextureBindGroup(PRIMARY, 1), data);


    Builds the following tree:

            TextureBindGroup(PRIMARY, 1);  -----------|
                       |                              |
            TextureBindGroup(SECONDARY, 2);       Triangles
                       |
                TransformGroup(matrix)
                       |
                   Triangles

*/
class RenderGroup {
public:
    typedef std::tr1::shared_ptr<RenderGroup> ptr;

    RenderGroup(const RenderGroup& parent);

    void set_parent_batcher(Batcher* batch);
};

class TransformGroup : public RenderGroup {};
class TextureBindGroup : public RenderGroup {};
class ShaderBindGroup : public RenderGroup {};

class Batcher {
public:
    void add(uint32_t count, GLenum type, const Group& group, ???) {

        RenderGroup& group = get_or_create_group(group);
    }

    void draw();

    RenderGroup& get_or_create_group(const RenderGroup& group);

private:
    RenderGroup::ptr groups_;

};

}

#endif // BATCHER_H_INCLUDED
