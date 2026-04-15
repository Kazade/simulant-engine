CleanupNode = smlt.define_node("cleanup_node")

function CleanupNode:on_clean_up()
    -- Mark cleanup by setting translation to a distinctive value.
    -- This will be visible through the C++ node if the callback ran.
    self.transform.translation = smlt.Vec3(77, 0, 0)
end
