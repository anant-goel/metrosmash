#pragma once
#include "GameMath.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Camera — extended with free-camera mode, smooth follow, FOV effects
// ─────────────────────────────────────────────────────────────────────────────
enum class CameraMode {
    THIRD_PERSON,   // original follow-cam
    FREE,           // detached fly camera (G to toggle)
    FIRST_PERSON,   // over-the-shoulder
    CINEMATIC       // slow orbit for screenshots
};

class Camera {
public:
    Vec3  position  {0, 8, 20};
    float yaw       = 0.f;
    float pitch     = -0.3f;
    float fov       = 70.f * DEG2RAD;
    float aspect    = 16.f / 9.f;
    float nearClip  = 0.1f;
    float farClip   = 600.f;

    // Third-person
    bool  thirdPerson = true;       // legacy flag — kept for compatibility
    float followDist  = 12.f;
    float followHeight= 6.f;

    // New camera system
    CameraMode mode   = CameraMode::THIRD_PERSON;
    float      fovTarget = 70.f * DEG2RAD;  // smooth FOV
    float      fovCurrent= 70.f * DEG2RAD;

    // Free-cam state
    Vec3  freeCamPos  {0, 30, 0};
    float freeCamSpeed= 20.f;
    bool  freeCamFast = false;

    // Cinematic orbit
    float cinematicAngle = 0.f;
    Vec3  cinematicTarget{0, 0, 0};
    float cinematicRadius= 40.f;
    float cinematicHeight= 15.f;

    // Smoothed position for third-person (no jitter)
    Vec3 smoothPos {0, 8, 20};
    float smoothSpeed = 12.f;

    Vec3 getForward() const {
        return Vec3(sinf(yaw)*cosf(pitch),
                    sinf(pitch),
                   -cosf(yaw)*cosf(pitch)).normalized();
    }
    Vec3 getRight() const {
        return Vec3(cosf(yaw), 0, sinf(yaw));
    }
    Vec3 getUp() const {
        return getRight().cross(getForward()).normalized();
    }

    Mat4 getView() const {
        Vec3 pos = (mode == CameraMode::FREE) ? freeCamPos : position;
        Vec3 target = pos + getForward();
        return Mat4::lookAt(pos, target, {0,1,0});
    }

    // View without translation (for sky dome)
    Mat4 getViewRotOnly() const {
        Vec3 f = getForward();
        Vec3 r = getRight();
        Vec3 u = r.cross(f).normalized();
        Mat4 v = Mat4::identity();
        v.m[0]=r.x; v.m[4]=r.y; v.m[8]=r.z;
        v.m[1]=u.x; v.m[5]=u.y; v.m[9]=u.z;
        v.m[2]=-f.x;v.m[6]=-f.y;v.m[10]=-f.z;
        return v;
    }

    Mat4 getProjection() const {
        return Mat4::perspective(fovCurrent, aspect, nearClip, farClip);
    }

    // Call once per frame to smooth FOV and position
    void update(float dt) {
        // Smooth FOV
        fovCurrent += (fovTarget - fovCurrent) * std::min(1.f, dt * 10.f);

        // Smooth follow position
        if (mode == CameraMode::THIRD_PERSON) {
            float lerpT = std::min(1.f, dt * smoothSpeed);
            smoothPos.x += (position.x - smoothPos.x) * lerpT;
            smoothPos.y += (position.y - smoothPos.y) * lerpT;
            smoothPos.z += (position.z - smoothPos.z) * lerpT;
        }

        // Cinematic orbit
        if (mode == CameraMode::CINEMATIC) {
            cinematicAngle += dt * 0.25f;
            position.x = cinematicTarget.x + cosf(cinematicAngle) * cinematicRadius;
            position.y = cinematicTarget.y + cinematicHeight;
            position.z = cinematicTarget.z + sinf(cinematicAngle) * cinematicRadius;
            Vec3 toTarget = (cinematicTarget - position).normalized();
            yaw   = atan2f(toTarget.x, -toTarget.z);
            pitch = asinf(toTarget.y);
        }
    }

    void followTarget(Vec3 target) {
        if (mode == CameraMode::FREE || mode == CameraMode::CINEMATIC) return;
        if (mode == CameraMode::FIRST_PERSON) {
            Vec3 eyeOffset(sinf(yaw)*0.3f, 1.6f, -cosf(yaw)*0.3f);
            position = target + eyeOffset;
            return;
        }
        // Third person
        Vec3 offset(
            -sinf(yaw) * followDist,
             followHeight,
             cosf(yaw)  * followDist
        );
        position = target + offset;

        // Collision-avoid: move camera closer if terrain between cam and target
        // (simple: clamp Y to above ground)
        if (position.y < 1.5f) position.y = 1.5f;
    }

    void moveFree(Vec3 delta) {
        if (mode != CameraMode::FREE) return;
        float speed = freeCamFast ? freeCamSpeed * 4.f : freeCamSpeed;
        Vec3 fwd = getForward();
        Vec3 rgt = getRight();
        Vec3 up  = {0, 1, 0};
        freeCamPos += rgt * delta.x * speed;
        freeCamPos += up  * delta.y * speed;
        freeCamPos += fwd * delta.z * speed;
    }

    void toggleMode() {
        switch (mode) {
            case CameraMode::THIRD_PERSON:
                mode = CameraMode::FREE;
                freeCamPos = position;
                thirdPerson = false;
                break;
            case CameraMode::FREE:
                mode = CameraMode::FIRST_PERSON;
                thirdPerson = false;
                break;
            case CameraMode::FIRST_PERSON:
                mode = CameraMode::THIRD_PERSON;
                thirdPerson = true;
                break;
            default:
                mode = CameraMode::THIRD_PERSON;
                thirdPerson = true;
                break;
        }
    }

    void rotate(float dYaw, float dPitch) {
        yaw   += dYaw;
        pitch  = clamp(pitch + dPitch,
                       mode == CameraMode::FREE ? -1.55f : -1.4f,
                       mode == CameraMode::FREE ?  1.55f :  0.4f);
    }

    // Speed-of-play FOV boost when moving fast
    void setSpeedFOV(float speedFraction) {
        float base = 70.f * DEG2RAD;
        fovTarget = base + speedFraction * 8.f * DEG2RAD;
    }

    // Get the effective camera position for rendering
    Vec3 getEffectivePos() const {
        if (mode == CameraMode::FREE) return freeCamPos;
        return position;
    }
};
