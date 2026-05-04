#include "Building.h"
#include <cmath>
#include <algorithm>

static const float BLOCK_SIZE = 1.2f;

Vec3 Building::styleColor(int style, int x, int y, int z) {
    // style: 0=office tower, 1=apartment, 2=warehouse, 3=skyscraper
    switch(style) {
    case 0: // glass office - blue-grey with glass panes
        if (x == 0 || z == 0) return {0.55f, 0.65f, 0.75f}; // glass
        return {0.45f, 0.50f, 0.55f}; // concrete
    case 1: // apartment - warm brick
        if (y % 3 == 0) return {0.65f, 0.60f, 0.55f}; // floor plate
        return {0.75f, 0.45f, 0.35f}; // brick
    case 2: // warehouse - grey metal
        if (y == 0) return {0.40f, 0.38f, 0.35f}; // base
        return {0.55f, 0.55f, 0.58f}; // corrugated metal
    case 3: // skyscraper - dark glass
        if (x % 2 == 0 && z % 2 == 0) return {0.20f, 0.30f, 0.45f}; // dark glass
        return {0.35f, 0.40f, 0.45f};
    default:
        return {0.6f, 0.6f, 0.6f};
    }
}

void Building::build(Vec3 pos, int w, int h, int d, int style) {
    origin = pos;
    width  = w;
    height = h;
    depth  = d;
    blocks.clear();
    blocks.reserve(w * h * d);

    for (int z = 0; z < d; z++)
    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
        // Hollow interior — only exterior walls + floor/ceiling for performance
        bool isExterior = (x==0 || x==w-1 || z==0 || z==d-1 || y==0);
        bool isWindow   = (y > 0 && y < h-1 && (y % 2 == 1) &&
                           (x==0 || x==w-1 || z==0 || z==d-1));

        if (!isExterior && y > 0) continue; // skip interior above ground

        Block b;
        b.position = pos + Vec3(
            (x - w*0.5f + 0.5f) * BLOCK_SIZE,
            y * BLOCK_SIZE + BLOCK_SIZE * 0.5f,
            (z - d*0.5f + 0.5f) * BLOCK_SIZE
        );
        b.color  = styleColor(style, x, y, z);
        b.health = (y == 0) ? 200.f : 100.f; // ground floor is stronger

        // Block type affects fragility
        b.type = isWindow ? 1 : (style == 2 ? 2 : 0);
        if (b.type == 1) { b.health *= 0.4f; b.color = {0.7f, 0.85f, 0.95f}; }

        // Physics body setup
        b.body.position  = b.position;
        b.body.halfSize  = Vec3(BLOCK_SIZE*0.5f, BLOCK_SIZE*0.5f, BLOCK_SIZE*0.5f);
        b.body.mass      = (b.type == 1) ? 5.f : 50.f;
        b.body.isStatic  = true;
        b.body.active    = true;

        blocks.push_back(b);
    }
}

void Building::applyDamage(Vec3 worldPos, float radius, float damage) {
    bool changed = false;
    for (auto& b : blocks) {
        if (b.destroyed) continue;
        Vec3 diff = b.position - worldPos;
        float dist = diff.length();
        if (dist < radius) {
            float falloff = 1.f - (dist / radius);
            float dmg = damage * falloff * falloff;
            b.health -= dmg;
            if (b.health <= 0) {
                b.destroyed = true;
                b.body.active = false;
                changed = true;
            }
        }
    }
    if (changed) checkSupport();
}

void Building::checkSupport() {
    // Mark blocks with no support below as loose (physics-driven)
    for (auto& b : blocks) {
        if (b.destroyed || b.loose) continue;
        // Ground-level blocks are always supported
        if (b.position.y < BLOCK_SIZE * 0.6f) continue;

        // Check if there's a block directly below
        bool supported = false;
        for (const auto& other : blocks) {
            if (&other == &b || other.destroyed) continue;
            Vec3 diff = b.position - other.position;
            if (std::abs(diff.x) < BLOCK_SIZE * 0.6f &&
                std::abs(diff.z) < BLOCK_SIZE * 0.6f &&
                diff.y > 0 && diff.y < BLOCK_SIZE * 1.5f) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            b.loose = true;
            b.body.isStatic = false;
            b.body.velocity = {0, -0.5f, 0}; // slight initial drop
        }
    }
}

void Building::update(float dt) {
    int destroyed = 0;
    for (auto& b : blocks) {
        if (b.destroyed) { destroyed++; continue; }
        if (b.loose) {
            b.position = b.body.position;
            // Destroy if fallen below world
            if (b.position.y < -20.f) {
                b.destroyed = true;
                b.body.active = false;
            }
        }
    }
    if (destroyed == (int)blocks.size()) fullyDestroyed = true;
}

void Building::collectBodies(std::vector<RigidBody*>& out) {
    for (auto& b : blocks) {
        if (!b.destroyed && b.loose) {
            out.push_back(&b.body);
        }
    }
}

float Building::destructionRatio() const {
    if (blocks.empty()) return 0.f;
    int count = 0;
    for (const auto& b : blocks) if (b.destroyed) count++;
    return (float)count / (float)blocks.size();
}
