ReloadNodeA = smlt.define_node("reload_node_a")

function ReloadNodeA:on_create()
    self.transform.translation = smlt.Vec3(100, 0, 0)
    return true
end
