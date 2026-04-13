MaterialNode = smlt.define_node("material_node")

function MaterialNode:on_create()
    -- Try to load a material. If assets is nil or material fails, this errors.
    local a = self.assets
    assert(a ~= nil, "self.assets is nil")
    self.assets_ok = true
    self.transform.translation = smlt.Vec3(1, 0, 0)
    return true
end
