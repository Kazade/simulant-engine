#pragma once

#include <cstdint>
#include <memory>

namespace smlt {
namespace _impl {
struct FixtureData;
}

class PhysicsBody;
class PhysicsService;

class Fixture {
private:
    Fixture(_impl::FixtureData* fixture);

    PhysicsBody* body_ = nullptr;
    uint16_t kind_ = 0;

public:
    friend class PrivateContactFilter;
    friend class ContactListIterator;

    Fixture(const Fixture&) = default;

    const PhysicsBody* body() const {
        return body_;
    }

    uint16_t kind() const {
        return kind_;
    }
};

} // namespace smlt
