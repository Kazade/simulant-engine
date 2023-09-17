#include "physics_body.h"

namespace smlt {

// FIXME: Calculate these values from real-world values!
const PhysicsMaterial PhysicsMaterial::WOOD(0.005, 0.4, 0.2);
const PhysicsMaterial PhysicsMaterial::RUBBER(0.001, 0.3, 0.8);
const PhysicsMaterial PhysicsMaterial::IRON(0.1, 0.2, 0.00001);
const PhysicsMaterial PhysicsMaterial::STONE(0.1, 0.8, 0.00001);

const static auto& w = PhysicsMaterial::WOOD;
const static auto& r = PhysicsMaterial::RUBBER;
const static auto& i = PhysicsMaterial::IRON;
const static auto& s = PhysicsMaterial::STONE;

const PhysicsMaterial PhysicsMaterial::WOOD_25(w.density * 0.25f, w.friction, w.bounciness);
const PhysicsMaterial PhysicsMaterial::WOOD_50(w.density * 0.50f, w.friction, w.bounciness);
const PhysicsMaterial PhysicsMaterial::WOOD_75(w.density * 0.75f, w.friction, w.bounciness);

const PhysicsMaterial PhysicsMaterial::RUBBER_25(r.density * 0.25f, r.friction, r.bounciness);
const PhysicsMaterial PhysicsMaterial::RUBBER_50(r.density * 0.50f, r.friction, r.bounciness);
const PhysicsMaterial PhysicsMaterial::RUBBER_75(r.density * 0.75f, r.friction, r.bounciness);

const PhysicsMaterial PhysicsMaterial::IRON_25(i.density * 0.25f, i.friction, i.bounciness);
const PhysicsMaterial PhysicsMaterial::IRON_50(i.density * 0.50f, i.friction, i.bounciness);
const PhysicsMaterial PhysicsMaterial::IRON_75(i.density * 0.75f, i.friction, i.bounciness);

const PhysicsMaterial PhysicsMaterial::STONE_25(s.density * 0.25f, s.friction, s.bounciness);
const PhysicsMaterial PhysicsMaterial::STONE_50(s.density * 0.50f, s.friction, s.bounciness);
const PhysicsMaterial PhysicsMaterial::STONE_75(s.density * 0.75f, s.friction, s.bounciness);

}
