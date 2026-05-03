#pragma once
#include <cmath>
#include <algorithm>

struct Vec3 {
    float x, y, z;
    Vec3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3 operator/(float s) const { return {x/s, y/s, z/s}; }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
    float dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x};
    }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    float lengthSq() const { return x*x + y*y + z*z; }
    Vec3 normalized() const {
        float l = length();
        if (l < 1e-6f) return {0,0,0};
        return *this / l;
    }
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a + (b - a) * t;
    }
};

struct Vec2 {
    float x, y;
    Vec2(float x=0, float y=0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s) const { return {x*s, y*s}; }
    float length() const { return std::sqrt(x*x + y*y); }
    Vec2 normalized() const {
        float l = length();
        if (l < 1e-6f) return {0,0};
        return *this / l;
    }
};

// 4x4 matrix (column-major for OpenGL)
struct Mat4 {
    float m[16] = {};

    static Mat4 identity() {
        Mat4 r;
        r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.f;
        return r;
    }

    static Mat4 translate(Vec3 t) {
        Mat4 r = identity();
        r.m[12]=t.x; r.m[13]=t.y; r.m[14]=t.z;
        return r;
    }

    static Mat4 scale(Vec3 s) {
        Mat4 r = identity();
        r.m[0]=s.x; r.m[5]=s.y; r.m[10]=s.z;
        return r;
    }

    static Mat4 rotateY(float angle) {
        Mat4 r = identity();
        float c=cosf(angle), s=sinf(angle);
        r.m[0]=c; r.m[2]=-s;
        r.m[8]=s; r.m[10]=c;
        return r;
    }

    static Mat4 rotateX(float angle) {
        Mat4 r = identity();
        float c=cosf(angle), s=sinf(angle);
        r.m[5]=c; r.m[6]=s;
        r.m[9]=-s; r.m[10]=c;
        return r;
    }

    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 r;
        float f = 1.f / tanf(fov * 0.5f);
        r.m[0]  = f / aspect;
        r.m[5]  = f;
        r.m[10] = (far + near) / (near - far);
        r.m[11] = -1.f;
        r.m[14] = (2.f * far * near) / (near - far);
        return r;
    }

    static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);
        Mat4 r = identity();
        r.m[0]=s.x;  r.m[4]=s.y;  r.m[8]=s.z;
        r.m[1]=u.x;  r.m[5]=u.y;  r.m[9]=u.z;
        r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z;
        r.m[12]=-s.dot(eye);
        r.m[13]=-u.dot(eye);
        r.m[14]=f.dot(eye);
        return r;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for(int col=0;col<4;col++)
            for(int row=0;row<4;row++) {
                float sum=0;
                for(int k=0;k<4;k++)
                    sum += m[k*4+row] * o.m[col*4+k];
                r.m[col*4+row]=sum;
            }
        return r;
    }
};

inline float clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

inline float lerp(float a, float b, float t) {
    return a + (b-a)*t;
}

const float PI = 3.14159265358979f;
const float DEG2RAD = PI / 180.f;
