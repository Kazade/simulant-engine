TransformChangeNode = smlt.define_node("transform_change_node")
TransformChangeNode._first_change = true

function TransformChangeNode:on_transformation_changed()
    if TransformChangeNode._first_change then
        TransformChangeNode._first_change = false
        self.transform.scale_factor = smlt.Vec3(2, 2, 2)
    end
end
