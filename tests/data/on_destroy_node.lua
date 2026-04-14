DestroyNode = smlt.define_node("destroy_node_node")

function DestroyNode:on_destroy()
    -- Return false to cancel destruction.
    return false
end
