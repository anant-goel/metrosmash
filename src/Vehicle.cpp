#include "pch.h"
#include "Vehicle.h"
#include <algorithm>
#include <cmath>

void Vehicle::init(Vec3 pos, VehicleType t) {
    position = pos;
    type     = t;
    body.position = pos;
    body.isStatic = false;
    body.active   = true;

    switch (t) {
    case VehicleType::CAR:
        body.halfSize   = {1.0f, 0.6f, 2.0f};
        body.mass       = 1200.f;
        maxSpeed        = 20.f;
        bodyColor       = {0.8f, 0.15f, 0.1f};
        accentColor     = {0.3f, 0.05f, 0.05f};
        break;
    case VehicleType::TANK:
        body.halfSize   = {1.4f, 0.9f, 2.8f};
        body.mass       = 8000.f;
        maxSpeed        = 10.f;
        bodyColor       = {0.3f, 0.4f, 0.2f};
        accentColor     = {0.2f, 0.3f, 0.1f};
        break;
    case VehicleType::BULLDOZER:
        body.halfSize   = {1.2f, 1.0f, 2.4f};
        body.mass       = 5000.f;
        maxSpeed        = 7.f;
        bodyColor       = {0.9f, 0.6f, 0.05f};
        accentColor     = {0.6f, 0.3f, 0.0f};
        break;
    }
    body.restitution = 0.15f;
    body.friction    = 0.8f;
}

void Vehicle::update(float dt, bool accel, bool brake, float steer) {
    if (destroyed) return;

    // Steering (only effective while moving)
    float turnRate = 2.0f * (std::abs(speed) / maxSpeed);
    yaw += steer * turnRate * dt;

    // Acceleration / braking
    float driveForce = 0.f;
    if (accel)  driveForce =  1.f;
    if (brake)  driveForce = -1.f;

    float accelRate = 8.f;
    float dragRate  = 4.f;

    speed += driveForce * accelRate * dt;

    // Drag / friction
    if (!accel && !brake) {
        speed *= (1.f - dragRate * dt);
        if (std::abs(speed) < 0.1f) speed = 0.f;
    }

    speed = std::max(-maxSpeed * 0.5f, std::min(maxSpeed, speed));

    // Move body in facing direction
    Vec3 fwd = getForward();
    body.velocity.x = fwd.x * speed;
    body.velocity.z = fwd.z * speed;

    // Keep grounded (simplified — no real suspension)
    body.position.y = body.halfSize.y;
    body.velocity.y = 0.f;

    position = body.position;
    body.position = position;
}

void Vehicle::ram(Vec3 dir, float force) {
    body.applyImpulse(dir * force);
}
