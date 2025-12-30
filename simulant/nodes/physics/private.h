#pragma once

#include "material.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct b3Body;
struct b3Hull;
struct b3Fixture;

namespace smlt {

class b3MeshGenerator;

namespace _impl {

struct FixtureData {
    b3Fixture* fixture = nullptr;
    PhysicsMaterial material;
    std::string name;
    uint16_t kind = 0;
    std::shared_ptr<b3MeshGenerator> mesh;
};

struct BounceFixtureData {
    b3Fixture* fixture = nullptr;
};

struct BounceData {
    b3Body* body = nullptr;
    std::vector<std::shared_ptr<b3Hull>> hulls;
    std::vector<std::shared_ptr<FixtureData>> fixtures;
};

} // namespace _impl
} // namespace smlt
