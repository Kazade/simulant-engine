MultiClassA = smlt.define_node("multi_class_a")

function MultiClassA:on_create()
    self.transform.translation = smlt.Vec3(1, 0, 0)
    return true
end

MultiClassB = smlt.define_node("multi_class_b")

function MultiClassB:on_create()
    self.transform.translation = smlt.Vec3(0, 2, 0)
    return true
end
