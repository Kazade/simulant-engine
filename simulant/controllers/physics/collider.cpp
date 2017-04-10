#include "collider.h"

namespace smlt {
namespace controllers {

// FIXME: Calculate these values from real-world values!
const PhysicsMaterial PhysicsMaterial::WOOD(0.005, 0.4, 0.01);
const PhysicsMaterial PhysicsMaterial::RUBBER(0.001, 0.3, 0.8);
const PhysicsMaterial PhysicsMaterial::IRON(0.1, 0.2, 0.00001);

}
}
