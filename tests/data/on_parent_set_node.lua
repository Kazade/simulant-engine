ParentSetNode = smlt.define_node("parent_set_node")

function ParentSetNode:on_parent_set()
    self.transform.translation = smlt.Vec3(99, 0, 0)
end
