#include "Player.h"
#include <functional>
#include <algorithm>

void Player::init() {
    body.position  = position;
    body.halfSize  = {0.4f, 0.9f, 0.4f};
    body.mass      = 70.f;
    body.restitution = 0.05f;
    body.friction  = 0.7f;
    body.active    = true;
    body.isStatic  = false;
}

void Player::update(float dt) {
    if (mode != PlayerMode::ON_FOOT) {
        position = body.position;
        return;
    }

    // Movement
    Vec3 dir {0,0,0};
    if (moveF) dir += getForward();
    if (moveB) dir -= getForward();
    if (moveR) dir += getRight();
    if (moveL) dir -= getRight();

    if (dir.lengthSq() > 0.01f) dir = dir.normalized();

    // Apply horizontal velocity directly for responsive feel
    body.velocity.x = dir.x * speed;
    body.velocity.z = dir.z * speed;

    // Jump
    if (jumping && body.onGround) {
        body.velocity.y = jumpForce;
        body.onGround = false;
    }

    // Update position from physics body
    position = body.position;

    // Update explosive fuses
    for (auto& e : placedExplosives) {
        if (!e.armed || e.detonated) continue;
        // Fuse-based explosives don't auto-detonate here
        // (manual detonator via wantsDetonate)
    }

    wantsPlace = false;
    jumping    = false;
}

void Player::placeExplosive() {
    if (explosiveCount <= 0) return;
    Explosive e;
    e.position  = position + getForward() * 2.f;
    e.position.y = 0.3f;
    e.fuseTime  = 3.f;
    e.radius    = 8.f;
    e.strength  = 600.f;
    e.armed     = true;
    placedExplosives.push_back(e);
    explosiveCount--;
}

void Player::detonateAll(std::vector<RigidBody*>& bodies,
                          std::function<void(Vec3,float,float)> onExplode) {
    for (auto& e : placedExplosives) {
        if (!e.armed || e.detonated) continue;
        e.detonated = true;
        onExplode(e.position, e.radius, e.strength);
    }
    placedExplosives.erase(
        std::remove_if(placedExplosives.begin(), placedExplosives.end(),
            [](const Explosive& e){ return e.detonated; }),
        placedExplosives.end());
}

void Player::enterVehicle(Vec3 vehiclePos) {
    mode = PlayerMode::IN_VEHICLE;
    body.active = false;
}

void Player::exitVehicle() {
    mode = PlayerMode::ON_FOOT;
    body.active = true;
    body.position = position;
}
