#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  GfxShaders.h  —  Advanced GLSL shader pipeline for Metro Smash
//  Implements: PBR lighting, shadow mapping, SSAO, HDR bloom, God rays,
//              dynamic sky with clouds, rain streaks, heat haze, vignette,
//              chromatic aberration, film grain, free camera
// ─────────────────────────────────────────────────────────────────────────────
#pragma once
#include "GameMath.h"
#include <string>
#include <unordered_map>
#include <vector>

// ── OpenGL function-pointer types for ARB/core extensions ────────────────────
#ifdef _WIN32
#  include <windows.h>
#endif
#include <GL/gl.h>

// Minimal GL extension loader declarations (resolved in GfxShaders.cpp)
extern void* gfxGetProcAddress(const char* name);

// GL types we need beyond what gl.h provides
typedef char            GLchar;
typedef ptrdiff_t       GLsizeiptr;
typedef ptrdiff_t       GLintptr;
#ifndef GL_VERSION_2_0
typedef unsigned int    GLuint;
typedef int             GLint;
typedef float           GLfloat;
typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef int             GLsizei;
#endif

// ── Constants ─────────────────────────────────────────────────────────────────
static const int SHADOW_MAP_SIZE  = 2048;
static const int SSAO_NOISE_SIZE  = 4;
static const int SSAO_KERNELS     = 32;
static const int BLOOM_DOWNSAMPLE = 4;

// ── Shader source strings (inline GLSL 1.30) ─────────────────────────────────
namespace GlslSrc {

// ── Geometry pass: G-Buffer (position, normal, albedo+roughness) ─────────────
static const char* GBUF_VERT = R"GLSL(
#version 130
attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec2 aUV;
attribute vec3 aColor;

varying vec3 vWorldPos;
varying vec3 vNormal;
varying vec2 vUV;
varying vec3 vColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMat;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal   = normalize(uNormalMat * aNormal);
    vUV       = aUV;
    vColor    = aColor;
    gl_Position = uProj * uView * worldPos;
}
)GLSL";

static const char* GBUF_FRAG = R"GLSL(
#version 130
varying vec3 vWorldPos;
varying vec3 vNormal;
varying vec2 vUV;
varying vec3 vColor;

uniform float uRoughness;
uniform float uMetallic;
uniform float uEmissive;
uniform vec3  uEmissiveColor;
uniform float uTime;
uniform int   uIsGlass;
uniform int   uIsWindow;

// Output to multiple render targets via gl_FragData
void main() {
    vec3 albedo = vColor;

    // Glass: slight blue tint + fresnel transparency suggestion
    if (uIsGlass == 1) {
        albedo = mix(albedo, vec3(0.6, 0.8, 0.95), 0.4);
    }

    // Window glow: emissive pulse
    vec3 emissive = uEmissiveColor * uEmissive;
    if (uIsWindow == 1) {
        float flicker = 0.95 + 0.05 * sin(uTime * 3.7 + vWorldPos.x * 0.3);
        emissive = vec3(1.0, 0.9, 0.6) * flicker * 0.8;
    }

    // Pack into 3 render targets:
    //   RT0 = albedo.rgb + roughness
    //   RT1 = world normal.xyz + metallic
    //   RT2 = world position.xyz + emissive intensity
    gl_FragData[0] = vec4(albedo, uRoughness);
    gl_FragData[1] = vec4(vNormal * 0.5 + 0.5, uMetallic);
    gl_FragData[2] = vec4(vWorldPos, length(emissive));
}
)GLSL";

// ── Shadow map pass ───────────────────────────────────────────────────────────
static const char* SHADOW_VERT = R"GLSL(
#version 130
attribute vec3 aPos;
uniform mat4 uLightMVP;
void main() {
    gl_Position = uLightMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* SHADOW_FRAG = R"GLSL(
#version 130
void main() {
    // Depth written automatically
}
)GLSL";

// ── Lighting pass: PBR + shadows + SSAO ──────────────────────────────────────
static const char* LIGHT_VERT = R"GLSL(
#version 130
attribute vec2 aPos;
varying vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)GLSL";

static const char* LIGHT_FRAG = R"GLSL(
#version 130
varying vec2 vUV;

uniform sampler2D uGAlbedo;     // RT0: albedo + roughness
uniform sampler2D uGNormal;     // RT1: normal + metallic
uniform sampler2D uGPosition;   // RT2: world pos + emissive
uniform sampler2D uShadowMap;
uniform sampler2D uSSAO;
uniform sampler2D uBlueNoise;

uniform vec3  uCamPos;
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform vec3  uSkyColor;
uniform vec3  uFogColor;
uniform float uFogDensity;
uniform mat4  uLightSpaceMat;
uniform float uTime;
uniform float uExposure;
uniform int   uEnableShadows;
uniform int   uEnableSSAO;

const float PI = 3.14159265;

// GGX Normal Distribution
float DistributionGGX(vec3 N, vec3 H, float rough) {
    float a  = rough * rough;
    float a2 = a * a;
    float NdH  = max(dot(N, H), 0.0);
    float NdH2 = NdH * NdH;
    float denom = (NdH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 0.0001);
}

// Smith-Schlick geometry
float GeometrySchlick(float NdV, float rough) {
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return NdV / (NdV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    return GeometrySchlick(max(dot(N,V),0.0), rough)
         * GeometrySchlick(max(dot(N,L),0.0), rough);
}

// Fresnel-Schlick
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// PCF shadow sampling (9-tap)
float sampleShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    if (uEnableShadows == 0) return 1.0;
    vec3 proj = fragPosLightSpace.xyz / fragPosLightSpace.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 1.0;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float shadow = 0.0;
    float texelSize = 1.0 / float(2048);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++) {
        float pcfDepth = texture2D(uShadowMap,
            proj.xy + vec2(x,y) * texelSize).r;
        shadow += (proj.z - bias > pcfDepth) ? 0.0 : 1.0;
    }
    return shadow / 9.0;
}

// Exponential fog
float fogFactor(float dist) {
    return exp(-uFogDensity * uFogDensity * dist * dist);
}

void main() {
    // Sample G-buffer
    vec4 gAlbedoR = texture2D(uGAlbedo,    vUV);
    vec4 gNormalM = texture2D(uGNormal,    vUV);
    vec4 gPosE    = texture2D(uGPosition,  vUV);

    vec3 albedo    = gAlbedoR.rgb;
    float rough    = clamp(gAlbedoR.a, 0.05, 1.0);
    vec3  normal   = normalize(gNormalM.rgb * 2.0 - 1.0);
    float metallic = gNormalM.a;
    vec3  worldPos = gPosE.rgb;
    float emissive = gPosE.a;

    // Skip empty fragments
    if (length(worldPos) < 0.001 && gPosE.a < 0.001) {
        gl_FragColor = vec4(uSkyColor, 1.0);
        return;
    }

    vec3 V = normalize(uCamPos - worldPos);
    vec3 L = normalize(-uSunDir);
    vec3 H = normalize(V + L);

    // PBR base reflectance
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, H, rough);
    float G   = GeometrySmith(normal, V, L, rough);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  numer  = NDF * G * F;
    float denom  = 4.0 * max(dot(normal,V),0.0) * max(dot(normal,L),0.0) + 0.0001;
    vec3  spec   = numer / denom;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdL = max(dot(normal, L), 0.0);

    // Shadow
    vec4 fragPosLS = uLightSpaceMat * vec4(worldPos, 1.0);
    float shadow = sampleShadow(fragPosLS, normal, L);

    // SSAO
    float ao = 1.0;
    if (uEnableSSAO == 1) ao = texture2D(uSSAO, vUV).r;

    // Ambient (sky hemisphere)
    float skyFactor = normal.y * 0.5 + 0.5;
    vec3 ambient = mix(uSkyColor * 0.3, uSkyColor * 0.6, skyFactor) * albedo * ao;

    // Directional sun
    vec3 directLight = (kD * albedo / PI + spec) * uSunColor * NdL * shadow;

    // Emissive add
    vec3 emissiveContrib = albedo * emissive * 2.0;

    vec3 color = ambient + directLight + emissiveContrib;

    // HDR tone mapping (ACES filmic)
    color *= uExposure;
    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);

    // Fog
    float dist2cam = length(worldPos - uCamPos);
    float ff = fogFactor(dist2cam);
    color = mix(uFogColor, color, ff);

    // Gamma correction
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));

    gl_FragColor = vec4(color, 1.0);
}
)GLSL";

// ── SSAO pass ─────────────────────────────────────────────────────────────────
static const char* SSAO_FRAG = R"GLSL(
#version 130
varying vec2 vUV;
uniform sampler2D uGPosition;
uniform sampler2D uGNormal;
uniform sampler2D uNoise;
uniform vec3  uKernel[32];
uniform mat4  uProj;
uniform vec2  uNoiseScale;
uniform float uRadius;
uniform float uBias;

void main() {
    vec3 fragPos = texture2D(uGPosition, vUV).rgb;
    vec3 normal  = normalize(texture2D(uGNormal, vUV).rgb * 2.0 - 1.0);
    vec3 noise   = normalize(texture2D(uNoise, vUV * uNoiseScale).rgb);

    vec3 tangent   = normalize(noise - normal * dot(noise, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < 32; i++) {
        vec3 samplePos = TBN * uKernel[i];
        samplePos = fragPos + samplePos * uRadius;

        vec4 offset = uProj * vec4(samplePos, 1.0);
        offset.xy /= offset.w;
        offset.xy  = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture2D(uGPosition, offset.xy).z;
        float rangeCheck  = smoothstep(0.0, 1.0, uRadius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + uBias ? 1.0 : 0.0) * rangeCheck;
    }
    gl_FragColor = vec4(vec3(1.0 - (occlusion / 32.0)), 1.0);
}
)GLSL";

// ── Bloom: bright-extract + blur ─────────────────────────────────────────────
static const char* BLOOM_EXTRACT_FRAG = R"GLSL(
#version 130
varying vec2 vUV;
uniform sampler2D uScene;
uniform float uThreshold;
void main() {
    vec3 c = texture2D(uScene, vUV).rgb;
    float brightness = dot(c, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > uThreshold)
        gl_FragColor = vec4(c, 1.0);
    else
        gl_FragColor = vec4(0.0);
}
)GLSL";

static const char* GAUSSIAN_BLUR_FRAG = R"GLSL(
#version 130
varying vec2 vUV;
uniform sampler2D uTex;
uniform vec2  uDirection;
uniform float uTexelSize;
void main() {
    float weights[5];
    weights[0]=0.227027; weights[1]=0.194595;
    weights[2]=0.121622; weights[3]=0.054054; weights[4]=0.016216;
    vec3 result = texture2D(uTex, vUV).rgb * weights[0];
    for (int i = 1; i < 5; i++) {
        vec2 offset = uDirection * float(i) * uTexelSize;
        result += texture2D(uTex, vUV + offset).rgb * weights[i];
        result += texture2D(uTex, vUV - offset).rgb * weights[i];
    }
    gl_FragColor = vec4(result, 1.0);
}
)GLSL";

// ── Final composite: scene + bloom + postfx ───────────────────────────────────
static const char* COMPOSITE_FRAG = R"GLSL(
#version 130
varying vec2 vUV;
uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform sampler2D uBlueNoise;
uniform float uTime;
uniform float uBloomStrength;
uniform float uChromaticAberration;
uniform float uVignetteStrength;
uniform float uFilmGrain;
uniform int   uEnableBloom;
uniform int   uEnableChromaticAberration;
uniform int   uEnableVignette;
uniform int   uEnableFilmGrain;
uniform int   uEnableDepthOfField;
uniform float uFocusDepth;
uniform float uDofStrength;

void main() {
    vec2 uv = vUV;
    vec3 color;

    // Chromatic aberration
    if (uEnableChromaticAberration == 1) {
        vec2 ca = (uv - 0.5) * uChromaticAberration;
        color.r = texture2D(uScene, uv + ca).r;
        color.g = texture2D(uScene, uv      ).g;
        color.b = texture2D(uScene, uv - ca).b;
    } else {
        color = texture2D(uScene, uv).rgb;
    }

    // Bloom
    if (uEnableBloom == 1) {
        vec3 bloom = texture2D(uBloom, uv).rgb;
        color += bloom * uBloomStrength;
    }

    // Vignette
    if (uEnableVignette == 1) {
        vec2 vig = uv - 0.5;
        float vf = 1.0 - dot(vig, vig) * uVignetteStrength * 2.0;
        color *= clamp(vf, 0.0, 1.0);
    }

    // Film grain
    if (uEnableFilmGrain == 1) {
        float noise = texture2D(uBlueNoise,
            uv * 3.7 + fract(vec2(uTime * 0.17, uTime * 0.29))).r;
        color += (noise - 0.5) * uFilmGrain;
    }

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
)GLSL";

// ── Sky dome shader ───────────────────────────────────────────────────────────
static const char* SKY_VERT = R"GLSL(
#version 130
attribute vec3 aPos;
varying vec3 vDir;
uniform mat4 uViewRot;  // view without translation
uniform mat4 uProj;
void main() {
    vDir = aPos;
    vec4 pos = uProj * uViewRot * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // keep at max depth
}
)GLSL";

static const char* SKY_FRAG = R"GLSL(
#version 130
varying vec3 vDir;
uniform vec3  uSunDir;
uniform vec3  uSkyZenith;
uniform vec3  uSkyHorizon;
uniform vec3  uGroundColor;
uniform float uTime;
uniform float uCloudSpeed;
uniform float uCloudCoverage;
uniform float uCloudSharpness;

// Fast hash
float hash(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453);
}

// Smooth noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1,0));
    float c = hash(i + vec2(0,1));
    float d = hash(i + vec2(1,1));
    return mix(mix(a,b,f.x), mix(c,d,f.x), f.y);
}

// Fractal Brownian Motion for clouds
float fbm(vec2 p) {
    float v = 0.0, amp = 0.5;
    for (int i = 0; i < 6; i++) {
        v   += noise(p) * amp;
        p   *= 2.1;
        amp *= 0.5;
    }
    return v;
}

// Preetham sky model (simplified)
vec3 preethamSky(vec3 dir) {
    float elevation = clamp(dir.y, 0.0, 1.0);
    float horizonFactor = pow(1.0 - elevation, 4.0);
    vec3 sky = mix(uSkyZenith, uSkyHorizon, horizonFactor);

    // Sun disk
    float sunAngle = dot(normalize(dir), normalize(-uSunDir));
    float sunDisk  = smoothstep(0.9990, 0.9995, sunAngle);
    float sunGlow  = pow(max(sunAngle, 0.0), 64.0) * 0.4;
    vec3  sunColor = vec3(1.0, 0.85, 0.5);
    sky += sunColor * sunDisk * 3.0;
    sky += sunColor * sunGlow;

    return sky;
}

void main() {
    vec3 dir = normalize(vDir);

    // Ground
    if (dir.y < -0.02) {
        gl_FragColor = vec4(uGroundColor, 1.0);
        return;
    }

    vec3 sky = preethamSky(dir);

    // Clouds on upper hemisphere
    if (dir.y > 0.05) {
        vec2 cloudUV = dir.xz / (dir.y + 0.1) * 0.15;
        cloudUV += vec2(uTime * uCloudSpeed * 0.003, uTime * uCloudSpeed * 0.001);

        float cloud = fbm(cloudUV * 3.0);
        cloud = smoothstep(uCloudCoverage - uCloudSharpness,
                           uCloudCoverage + uCloudSharpness, cloud);

        // Light the clouds
        float sunDot = max(dot(normalize(dir), normalize(-uSunDir)), 0.0);
        vec3 cloudColor = mix(vec3(0.7, 0.72, 0.78),   // shadow
                              vec3(1.0, 0.98, 0.95),    // lit
                              sunDot);
        sky = mix(sky, cloudColor, cloud * 0.9);
    }

    gl_FragColor = vec4(sky, 1.0);
}
)GLSL";

// ── Rain streak particle shader ───────────────────────────────────────────────
static const char* RAIN_VERT = R"GLSL(
#version 130
attribute vec3 aPos;
attribute float aAlpha;
varying float vAlpha;
uniform mat4 uMVP;
void main() {
    vAlpha = aAlpha;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* RAIN_FRAG = R"GLSL(
#version 130
varying float vAlpha;
uniform vec3 uRainColor;
void main() {
    gl_FragColor = vec4(uRainColor, vAlpha);
}
)GLSL";

// ── Heat haze (screen-space distortion) ──────────────────────────────────────
static const char* HAZE_FRAG = R"GLSL(
#version 130
varying vec2 vUV;
uniform sampler2D uScene;
uniform sampler2D uNoise;
uniform float uTime;
uniform float uHazeStrength;
void main() {
    vec2 noise = texture2D(uNoise,
        vUV * 4.0 + vec2(uTime * 0.05, uTime * 0.03)).rg * 2.0 - 1.0;
    vec2 distorted = vUV + noise * uHazeStrength;
    gl_FragColor = texture2D(uScene, distorted);
}
)GLSL";

} // namespace GlslSrc

// ─────────────────────────────────────────────────────────────────────────────
//  ShaderProgram: compile + link + uniform helpers
// ─────────────────────────────────────────────────────────────────────────────
struct ShaderProgram {
    unsigned int id = 0;

    bool compile(const char* vert, const char* frag);
    void use() const;
    void setInt  (const char* name, int v)        const;
    void setFloat(const char* name, float v)      const;
    void setVec2 (const char* name, float x, float y) const;
    void setVec3 (const char* name, float x, float y, float z) const;
    void setVec3v(const char* name, const float* v, int count) const;
    void setMat4 (const char* name, const float* m) const;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Framebuffer helper
// ─────────────────────────────────────────────────────────────────────────────
struct Framebuffer {
    unsigned int fbo = 0;
    unsigned int colorTex[4] = {};
    unsigned int depthTex = 0;
    unsigned int depthRbo = 0;
    int numColorAttachments = 1;
    int width = 0, height = 0;

    bool create(int w, int h, int numColor, bool hasDepthTex = false,
                bool isFloat = true);
    void bind() const;
    void unbind() const;
    void destroy();
};

// ─────────────────────────────────────────────────────────────────────────────
//  GfxPipeline: manages all render passes
// ─────────────────────────────────────────────────────────────────────────────
struct GfxSettings {
    // Resolution & quality
    int   renderScale    = 100;    // percent of window size
    int   shadowQuality  = 2;      // 0=off 1=low 2=medium 3=high

    // Post-processing toggles
    bool  enableShadows  = true;
    bool  enableSSAO     = true;
    bool  enableBloom    = true;
    bool  enableHDR      = true;
    bool  enableChromaticAberration = true;
    bool  enableVignette = true;
    bool  enableFilmGrain= true;
    bool  enableRain     = false;
    bool  enableHeatHaze = false;
    bool  enableMotionBlur = false;

    // Sky
    float cloudCoverage  = 0.45f;
    float cloudSharpness = 0.15f;
    float cloudSpeed     = 1.0f;
    float sunPitch       = 0.4f;   // elevation angle of sun
    float sunYaw         = 1.2f;
    float timeOfDay      = 0.6f;   // 0=midnight 0.5=dawn 1=noon

    // Lighting
    float exposure       = 1.2f;
    float bloomStrength  = 0.25f;
    float bloomThreshold = 0.85f;
    float ssaoRadius     = 0.5f;
    float ssaoBias       = 0.025f;
    float fogDensity     = 0.006f;

    // Post
    float chromaticAberration = 0.0015f;
    float vignetteStrength    = 0.45f;
    float filmGrain           = 0.025f;

    // Rain
    float rainIntensity  = 0.7f;
    float rainAngle      = 0.15f;  // radians from vertical
};

struct RainSystem {
    static const int MAX_DROPS = 4000;
    struct Drop { Vec3 pos; float life; float alpha; };
    std::vector<Drop> drops;
    float spawnTimer = 0.f;

    void init(int count);
    void update(float dt, Vec3 camPos, float intensity, float angle);
    void reset(Drop& d, Vec3 camPos);
};

class GfxPipeline {
public:
    GfxSettings settings;
    bool         initialized = false;
    bool         fallbackMode = false; // use legacy GL if shaders fail

    bool init(int w, int h);
    void resize(int w, int h);
    void destroy();

    // Called from Renderer: wraps render passes
    // Pass 1: geometry (shadow + gbuffer)
    // Pass 2: lighting (PBR + SSAO)
    // Pass 3: post-processing (bloom, DoF, composite)
    void beginShadowPass();
    void endShadowPass();
    void beginGeometryPass();
    void endGeometryPass();
    void runLightingPass(Vec3 camPos, Vec3 sunDir, float gameTime);
    void runPostProcessing(float gameTime);
    void drawSky(const float* viewMat, const float* projMat, Vec3 camPos);
    void updateRain(float dt, Vec3 camPos);
    void drawRain(const float* mvp);

    // Bind the correct shader for geometry rendering
    // Returns false if should use legacy GL
    bool bindGeometryShader();
    bool bindShadowShader();

    // Set per-object uniforms
    void setObjectUniforms(const float* modelMat, Vec3 color,
                           float roughness, float metallic,
                           float emissive, bool isGlass, bool isWindow);

    // Getters for active shader
    ShaderProgram& geomShader()    { return sGeom;    }
    ShaderProgram& shadowShader()  { return sShadow;  }
    ShaderProgram& lightShader()   { return sLight;   }
    ShaderProgram& skyShader()     { return sSky;     }
    ShaderProgram& bloomShader()   { return sBloom;   }
    ShaderProgram& compositeShader(){ return sComposite; }

    // Sun direction from time-of-day
    Vec3 getSunDir() const;
    Vec3 getSkyZenith() const;
    Vec3 getSkyHorizon() const;
    Vec3 getFogColor() const;

private:
    int screenW = 1280, screenH = 720;
    int renderW = 1280, renderH = 720;

    // Framebuffers
    Framebuffer fbGBuffer;    // G-buffer (3 MRTs)
    Framebuffer fbShadow;     // Shadow map
    Framebuffer fbSSAO;       // SSAO
    Framebuffer fbSSAOBlur;   // Blurred SSAO
    Framebuffer fbLight;      // Lit scene
    Framebuffer fbBloomA;     // Bloom ping-pong A
    Framebuffer fbBloomB;     // Bloom ping-pong B
    Framebuffer fbHaze;       // Heat haze

    // Shaders
    ShaderProgram sGeom;
    ShaderProgram sShadow;
    ShaderProgram sLight;
    ShaderProgram sSky;
    ShaderProgram sSSAO;
    ShaderProgram sGaussian;
    ShaderProgram sBloomExtract;
    ShaderProgram sBloom;
    ShaderProgram sComposite;
    ShaderProgram sRain;
    ShaderProgram sHaze;

    // Textures
    unsigned int texSSAONoise = 0;
    unsigned int texBlueNoise = 0;
    unsigned int texSkyLUT    = 0;
    unsigned int vaoQuad      = 0;
    unsigned int vboQuad      = 0;
    unsigned int vaoSkyDome   = 0;
    unsigned int vboSkyDome   = 0;

    // Light space matrix (for shadows)
    float lightSpaceMat[16] = {};

    // SSAO kernel
    float ssaoKernel[SSAO_KERNELS * 3] = {};

    // Rain
    RainSystem rain;

    bool initShaders();
    bool initFramebuffers();
    void initSSAOKernel();
    void generateNoiseTextures();
    void buildQuadVAO();
    void buildSkyDome();
    void updateLightSpaceMatrix(Vec3 sunDir, Vec3 camPos);
    void drawQuad();
    void runSSAO(const float* projMat);
    void runBloom();
};

// Global pipeline instance (accessed from Renderer)
extern GfxPipeline gGfx;
