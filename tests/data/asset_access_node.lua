AssetNode = smlt.define_node("asset_node")

function AssetNode:on_create()
    local a = self.assets
    if a ~= nil then
        self.transform.translation = smlt.Vec3(42, 0, 0)
        return true
    end
    return false
end
