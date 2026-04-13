ReloadNodeB = smlt.define_node("reload_node_b")

function ReloadNodeB:on_create()
    self.transform.translation = smlt.Vec3(200, 0, 0)
    return true
end
