FileParamNode = smlt.define_node("file_param_node")
FileParamNode.params = {
    distance = smlt.define_node_param(smlt.NodeParamType.Float, "Distance from center"),
    height   = smlt.define_node_param(smlt.NodeParamType.Float, "Height above ground", 5.0)
}

function FileParamNode:on_create(params)
    self.transform.translation = smlt.Vec3(params.distance, params.height, 0)
    return true
end
