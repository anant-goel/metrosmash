#pragma once
#include "Math.h"

class Camera {
public:
    Vec3 position  {0, 8, 20};
    float yaw      = 0.f;     // horizontal rotation (radians)
    float pitch    = -0.3f;   // vertical rotation (radians)
    float fov      = 70.f * DEG2RAD;
    float aspect   = 16.f/9.f;
    float nearClip = 0.1f;
    float farClip  = 500.f;

    // Third-person follow settings
    bool  thirdPerson = true;
    float followDist  = 12.f;
    float followHeight= 6.f;

    Vec3 getForward() const {
        return Vec3(sinf(yaw)*cosf(pitch), sinf(pitch), -cosf(yaw)*cosf(pitch)).normalized();
    }
    Vec3 getRight() const {
        return Vec3(cosf(yaw), 0, sinf(yaw));
    }

    Mat4 getView() const {
        Vec3 target = position + getForward();
        return Mat4::lookAt(position, target, {0,1,0});
    }
    Mat4 getProjection() const {
        return Mat4::perspective(fov, aspect, nearClip, farClip);
    }

    void followTarget(Vec3 target) {
        if (!thirdPerson) return;
        Vec3 offset(
            -sinf(yaw) * followDist,
             followHeight,
             cosf(yaw)  * followDist
        );
        position = target + offset;
    }

    void rotate(float dYaw, float dPitch) {
        yaw   += dYaw;
        pitch  = clamp(pitch + dPitch, -1.4f, 0.2f);
    }
};
