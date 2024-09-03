#include "fixture.h"
#include "bounce/dynamics/fixture.h"
#include "private.h"

namespace smlt {

Fixture::Fixture(_impl::FixtureData* fixture) {
    body_ = (PhysicsBody*)(fixture->fixture->GetBody()->GetUserData());
    kind_ = fixture->kind;
}

} // namespace smlt
