LateUpdateNode = smlt.define_node("late_update_node")

function LateUpdateNode:on_late_update(dt)
    self.transform.rotation_2d = smlt.Degrees(dt * 10)
end
