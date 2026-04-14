FindNode = smlt.define_node("find_node")

function FindNode:on_create()
    local a = self.assets
    assert(a ~= nil, "self.assets is nil")

    -- Mesh: load, rename, find back
    local mesh = a:load_mesh("assets/samples/cube.obj")
    assert(mesh ~= nil, "load_mesh should return a valid mesh")
    mesh:set_name("my_cube")
    local found_mesh = a:find_mesh("my_cube")
    assert(found_mesh ~= nil, "find_mesh should return the mesh")
    assert(found_mesh:name() == "my_cube", "found mesh name should match")

    -- Texture: load, rename, find back
    local tex = a:load_texture("assets/samples/crate.png")
    if tex ~= nil then
        tex:set_name("my_tex")
        local found_tex = a:find_texture("my_tex")
        assert(found_tex ~= nil, "find_texture should return the texture")
    end

    -- Sound: load, rename, find back
    local snd = a:load_sound("assets/sounds/simulant.ogg")
    if snd ~= nil then
        snd:set_name("my_sound")
        local found_snd = a:find_sound("my_sound")
        assert(found_snd ~= nil, "find_sound should return the sound")
    end

    self.transform.translation = smlt.Vec3(42, 0, 0)
    return true
end
