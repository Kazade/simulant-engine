#pragma once

#include "../asset.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../types.h"

namespace smlt {

class Binary:
    public Asset,
    public RefCounted<Binary>,
    public generic::Identifiable<BinaryID> {

public:
    Binary(BinaryID id, AssetManager* asset_manager, const std::vector<uint8_t>&& data):
        Asset(asset_manager),
        generic::Identifiable<BinaryID>(id),
        data_(std::move(data)) {}

    const uint8_t* data() const {
        return &data_[0];
    }

    std::size_t data_size_in_bytes() const {
        return data_.size();
    }

private:
    std::vector<uint8_t> data_;
};

}
