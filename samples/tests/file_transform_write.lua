FileTransformWriteNode = smlt.define_node("file_transform_write_node")

function FileTransformWriteNode:on_create()
    self.transform.translation = smlt.Vec3(7, 8, 9)
    return true
end
