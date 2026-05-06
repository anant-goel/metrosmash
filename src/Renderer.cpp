// ─────────────────────────────────────────────────────────────────────────────
//  Renderer.cpp — Advanced graphics pipeline for Metro Smash
//  Integrates GfxPipeline (PBR shaders, shadow maps, SSAO, bloom, HDR,
//  dynamic sky, rain) while preserving ALL original draw calls as fallback.
// ─────────────────────────────────────────────────────────────────────────────
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

// SFML OpenGL headers FIRST (before anything that touches cmath)
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>

#include "Renderer.h"
#include "Camera.h"
#include "GfxShaders.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Init
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::init(unsigned int width, unsigned int height) {
    screenW = width; screenH = height;
    setupGL();

    // Initialise the advanced pipeline — it gracefully falls back
    // to legacy GL if shaders or framebuffers aren't supported
    gGfx.settings.enableShadows             = gfxPanel.shadowsEnabled;
    gGfx.settings.enableSSAO               = gfxPanel.ssaoEnabled;
    gGfx.settings.enableBloom              = gfxPanel.bloomEnabled;
    gGfx.settings.enableVignette           = gfxPanel.vignetteOn;
    gGfx.settings.enableFilmGrain          = gfxPanel.grainOn;
    gGfx.settings.enableChromaticAberration= gfxPanel.chromaticOn;
    gGfx.settings.timeOfDay                = gfxPanel.timeOfDay;
    gGfx.settings.fogDensity               = gfxPanel.fogDensity;
    gGfx.settings.exposure                 = gfxPanel.exposure;
    gGfx.settings.bloomStrength            = gfxPanel.bloomStrength;
    gGfx.init(width, height);
}

void Renderer::setupGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    GLfloat lightPos[]  = {100.f, 180.f, 80.f, 0.f};   // directional (w=0)
    GLfloat lightDiff[] = {1.0f,  0.93f, 0.82f, 1.f};
    GLfloat lightAmb[]  = {0.28f, 0.30f, 0.38f, 1.f};
    GLfloat lightSpec[] = {0.6f,  0.6f,  0.6f,  1.f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiff);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);

    glViewport(0, 0, screenW, screenH);
}

void Renderer::resize(unsigned int w, unsigned int h) {
    screenW = w; screenH = h;
    gGfx.resize(w, h);
    glViewport(0, 0, w, h);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Matrix helpers
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::setMatrices(const Camera& cam) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glLoadMatrixf(cam.getProjection().m);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLoadMatrixf(cam.getView().m);
}

void Renderer::setModelMatrix(Vec3 pos, float yawAngle, Vec3 scl) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    if (yawAngle != 0.f)
        glRotatef(-yawAngle * 180.f / PI, 0, 1, 0);
    if (scl.x != 1.f || scl.y != 1.f || scl.z != 1.f)
        glScalef(scl.x, scl.y, scl.z);
}

void Renderer::pushIdentityModel() {
    glPushMatrix();
    glLoadIdentity();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Main render — orchestrates all passes
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::render(const World& world, const Camera& camera,
                      const sf::RenderWindow& /*win*/) {
    gameTime += dt;

    // Sync settings from panel each frame
    if (gGfx.initialized) {
        gGfx.settings.enableShadows              = gfxPanel.shadowsEnabled && gfxPanel.shadersEnabled;
        gGfx.settings.enableSSAO                = gfxPanel.ssaoEnabled    && gfxPanel.shadersEnabled;
        gGfx.settings.enableBloom               = gfxPanel.bloomEnabled   && gfxPanel.shadersEnabled;
        gGfx.settings.enableVignette            = gfxPanel.vignetteOn;
        gGfx.settings.enableFilmGrain           = gfxPanel.grainOn;
        gGfx.settings.enableChromaticAberration = gfxPanel.chromaticOn;
        gGfx.settings.enableRain                = gfxPanel.rainEnabled;
        gGfx.settings.timeOfDay                 = gfxPanel.timeOfDay;
        gGfx.settings.fogDensity                = gfxPanel.fogDensity;
        gGfx.settings.exposure                  = gfxPanel.exposure;
        gGfx.settings.bloomStrength             = gfxPanel.bloomStrength;
    }

    bool useShaders = gfxPanel.shadersEnabled &&
                      gGfx.initialized        &&
                      !gGfx.fallbackMode;

    if (useShaders) {
        renderAdvanced(world, camera);
    } else {
        renderLegacy(world, camera);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  ADVANCED RENDER PATH (PBR + shadows + SSAO + bloom + HDR)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::renderAdvanced(const World& world, const Camera& camera) {
    Vec3 camPos = camera.getEffectivePos();
    Vec3 sunDir = gGfx.getSunDir();

    // Update sun direction in legacy GL too
    GLfloat lp[4] = {-sunDir.x * 500.f, sunDir.y * 500.f, -sunDir.z * 500.f, 0.f};
    glLightfv(GL_LIGHT0, GL_POSITION, lp);

    // ── Pass 1: Shadow map ──────────────────────────────────────────────────
    if (gGfx.settings.enableShadows) {
        gGfx.beginShadowPass();
        renderShadowPass(world, camera);
        gGfx.endShadowPass();
    }

    // ── Pass 2: G-Buffer geometry ───────────────────────────────────────────
    gGfx.beginGeometryPass();
    setMatrices(camera);

    // Sky is drawn in lighting pass, skip in gbuffer pass

    // Ground
    drawGroundAdvanced();

    // Buildings
    for (const auto& b : world.buildings)
        drawBuildingAdvanced(b);

    // Vehicles
    for (const auto& v : world.vehicles)
        if (!v->destroyed) drawVehicleAdvanced(*v);

    // Player
    if (world.player.mode == PlayerMode::ON_FOOT)
        drawPlayerAdvanced(world.player, camera);

    gGfx.endGeometryPass();

    // ── Pass 3: Lighting (PBR + SSAO + shadows) ─────────────────────────────
    gGfx.runLightingPass(camPos, sunDir, gameTime);

    // ── Pass 4: Forward pass into lighting FBO (particles, FX, sky) ─────────
    // Bind lighting FBO for forward rendering
    gGfx.fbLight.bind();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    // Blit depth from g-buffer so forward objects depth-test correctly
    // (simple: re-draw sky + particles on top)

    // Sky dome
    gGfx.drawSky(camera.getViewRotOnly().m, camera.getProjection().m, camPos);

    // Particles / FX in forward pass (unchanged)
    setMatrices(camera);
    drawParticles(world.particles);
    drawExplosionFX(world.physics.activeExplosions);
    drawExplosiveMarkers(world.player);
    drawShockwaves(world.anim.shockwaves);
    drawDebrisChunks(world.anim.debris);

    // Rain
    gGfx.updateRain(dt, camPos);
    {
        Mat4 mvp = camera.getProjection() * camera.getView();
        gGfx.drawRain(mvp.m);
    }

    gGfx.fbLight.unbind();

    // ── Pass 5: Post-processing (bloom + composite) ──────────────────────────
    gGfx.runPostProcessing(gameTime);

    // Restore default GL state for SFML UI pass
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  LEGACY RENDER PATH — original GL fixed-function (unchanged behaviour)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::renderLegacy(const World& world, const Camera& camera) {
    // Sky colour based on time of day
    float tod = gfxPanel.timeOfDay;
    float sr = 0.35f + tod * 0.15f;
    float sg = 0.50f + tod * 0.20f;
    float sb = 0.65f + tod * 0.25f;
    glClearColor(sr, sg, sb, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setMatrices(camera);
    drawSkyboxLegacy();
    drawGround(250.f);

    for (const auto& b : world.buildings)
        drawBuilding(b);
    for (const auto& v : world.vehicles)
        if (!v->destroyed) drawVehicle(*v);
    if (world.player.mode == PlayerMode::ON_FOOT)
        drawPlayer(world.player, camera);

    drawParticles(world.particles);
    drawExplosionFX(world.physics.activeExplosions);
    drawExplosiveMarkers(world.player);
    drawShockwaves(world.anim.shockwaves);
    drawDebrisChunks(world.anim.debris);

    // Legacy rain (simple lines)
    if (gfxPanel.rainEnabled) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glColor4f(0.65f, 0.72f, 0.85f, 0.35f);
        Vec3 cam = camera.getEffectivePos();
        glBegin(GL_LINES);
        static std::vector<Vec3> rDrops;
        if (rDrops.empty()) {
            rDrops.resize(1500);
            for (auto& d : rDrops)
                d = {cam.x + (rand()%120-60)*1.f,
                     (rand()%30)*1.f,
                     cam.z + (rand()%120-60)*1.f};
        }
        for (auto& d : rDrops) {
            d.y -= 25.f * dt;
            d.x += sinf(gfxPanel.timeOfDay * 10.f) * 2.f * dt;
            if (d.y < 0.f) {
                d.y = 25.f + (rand()%10)*1.f;
                d.x = cam.x + (rand()%120-60)*1.f;
                d.z = cam.z + (rand()%120-60)*1.f;
            }
            glVertex3f(d.x, d.y, d.z);
            glVertex3f(d.x + sinf(gfxPanel.timeOfDay*10)*0.3f, d.y - 0.7f, d.z);
        }
        glEnd();
        glEnable(GL_LIGHTING);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Shadow pass geometry — minimal draw (positions only)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::renderShadowPass(const World& world, const Camera& /*cam*/) {
    for (const auto& b : world.buildings)
        drawBuildingShadow(b);
    // Vehicles
    for (const auto& v : world.vehicles) {
        if (v->destroyed) continue;
        glPushMatrix();
        glTranslatef(v->position.x, v->position.y, v->position.z);
        glRotatef(-v->yaw * 180.f / PI, 0, 1, 0);
        Vec3 hs = v->body.halfSize;
        drawCube({0,0,0}, hs, {1,1,1});
        glPopMatrix();
    }
}

void Renderer::drawBuildingShadow(const Building& b) {
    for (const auto& blk : b.blocks) {
        if (blk.destroyed) continue;
        drawCube(blk.position, blk.body.halfSize, {1,1,1});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Advanced geometry draw helpers (set PBR uniforms via GfxPipeline)
// ─────────────────────────────────────────────────────────────────────────────

// Helper: build a GL model matrix and push it as a shader uniform
static void buildModelMat(float out[16], Vec3 pos, float yaw = 0.f) {
    // Column-major 4x4
    float c = cosf(yaw), s = sinf(yaw);
    out[0]=c;   out[4]=0;  out[8]=-s;  out[12]=pos.x;
    out[1]=0;   out[5]=1;  out[9]=0;   out[13]=pos.y;
    out[2]=s;   out[6]=0;  out[10]=c;  out[14]=pos.z;
    out[3]=0;   out[7]=0;  out[11]=0;  out[15]=1;
}

void Renderer::drawGroundAdvanced() {
    float model[16]; buildModelMat(model, {0,0,0});
    gGfx.setObjectUniforms(model, {0.28f,0.30f,0.26f}, 0.95f, 0.0f, 0.f, false, false);

    glDisable(GL_LIGHTING);
    int tiles = 40; float sz = 250.f, tileSize = sz * 2.f / tiles;
    glBegin(GL_QUADS);
    for (int x = 0; x < tiles; x++)
    for (int z = 0; z < tiles; z++) {
        float wx = -sz + x * tileSize;
        float wz = -sz + z * tileSize;
        bool road = (x % 5 == 2 || z % 5 == 2);
        if (road) glColor3f(0.22f, 0.22f, 0.24f);
        else      glColor3f(0.28f, 0.30f, 0.26f);
        glNormal3f(0,1,0);
        glVertex3f(wx,          0, wz);
        glVertex3f(wx+tileSize, 0, wz);
        glVertex3f(wx+tileSize, 0, wz+tileSize);
        glVertex3f(wx,          0, wz+tileSize);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void Renderer::drawBuildingAdvanced(const Building& b) {
    for (const auto& blk : b.blocks) {
        if (blk.destroyed) continue;
        // Classify block type for PBR parameters
        Vec3 col = blk.color;
        float rough = 0.75f, metal = 0.1f, emis = 0.f;
        bool isGlass = false, isWindow = false;

        // Detect window-like blocks: bright/light blue tones
        if (col.y > 0.55f && col.z > 0.70f) {
            isGlass = true; rough = 0.05f; metal = 0.85f;
        }
        // Detect emissive (window glow): warm yellow/orange blocks
        if (col.x > 0.8f && col.y > 0.75f && col.z < 0.6f) {
            isWindow = true; emis = 0.6f;
        }
        // Concrete/stone
        if (col.x > 0.35f && col.y > 0.35f && col.z > 0.35f
            && std::abs(col.x-col.y)<0.15f) {
            rough = 0.88f; metal = 0.0f;
        }
        // Metal/steel
        if (col.x < 0.45f && col.y < 0.45f && col.z > 0.40f) {
            rough = 0.35f; metal = 0.7f;
        }

        float mdl[16]; buildModelMat(mdl, {0,0,0});
        gGfx.setObjectUniforms(mdl, col, rough, metal, emis, isGlass, isWindow);
        drawCube(blk.position, blk.body.halfSize, col);
    }
}

void Renderer::drawVehicleAdvanced(const Vehicle& v) {
    float mdl[16]; buildModelMat(mdl, v.position, v.yaw);
    gGfx.setObjectUniforms(mdl, v.bodyColor, 0.4f, 0.6f, 0.f, false, false);

    glPushMatrix();
    glTranslatef(v.position.x, v.position.y, v.position.z);
    glRotatef(-v.yaw * 180.f / PI, 0, 1, 0);

    Vec3 hs = v.body.halfSize;
    drawCube({0,0,0}, hs, v.bodyColor);
    Vec3 cab = {hs.x*0.7f, hs.y*0.5f, hs.z*0.5f};
    drawCube({0, hs.y+cab.y, 0}, cab, v.accentColor);

    // Windscreen glass
    gGfx.setObjectUniforms(mdl, {0.5f,0.65f,0.85f}, 0.05f, 0.9f, 0.f, true, false);
    drawCube({0, hs.y*0.6f, hs.z+0.05f}, {hs.x*0.65f, hs.y*0.4f, 0.05f}, {0.5f,0.65f,0.85f}, 0.6f);

    // Headlights emissive
    gGfx.setObjectUniforms(mdl, {1,0.95f,0.8f}, 0.1f, 0.8f, 2.0f, false, false);
    drawCube({-hs.x*0.6f, -hs.y*0.2f, hs.z+0.05f}, {0.12f,0.1f,0.05f}, {1,0.95f,0.8f});
    drawCube({ hs.x*0.6f, -hs.y*0.2f, hs.z+0.05f}, {0.12f,0.1f,0.05f}, {1,0.95f,0.8f});

    // Wheels (black rubber)
    gGfx.setObjectUniforms(mdl, {0.1f,0.1f,0.12f}, 0.9f, 0.0f, 0.f, false, false);
    Vec3 wc = {0.12f, 0.12f, 0.12f};
    float wy = -hs.y + 0.15f;
    drawCube({-hs.x-0.12f, wy,  hs.z*0.5f}, {0.12f,0.28f,0.32f}, wc);
    drawCube({ hs.x+0.12f, wy,  hs.z*0.5f}, {0.12f,0.28f,0.32f}, wc);
    drawCube({-hs.x-0.12f, wy, -hs.z*0.5f}, {0.12f,0.28f,0.32f}, wc);
    drawCube({ hs.x+0.12f, wy, -hs.z*0.5f}, {0.12f,0.28f,0.32f}, wc);

    if (v.type == VehicleType::BULLDOZER) {
        gGfx.setObjectUniforms(mdl, {0.7f,0.5f,0.1f}, 0.5f, 0.4f, 0.f, false, false);
        drawCube({0, 0, hs.z+0.3f}, {hs.x*1.1f, hs.y*0.8f, 0.2f}, {0.7f,0.5f,0.1f});
    }
    glPopMatrix();
}

void Renderer::drawPlayerAdvanced(const Player& p, const Camera& cam) {
    Vec3 diff = p.position - cam.getEffectivePos();
    if (diff.length() < 2.5f) return;

    float mdl[16]; buildModelMat(mdl, p.position, p.yaw);

    glPushMatrix();
    glTranslatef(p.position.x, p.position.y, p.position.z);
    glRotatef(-p.yaw * 180.f / PI, 0, 1, 0);

    // Clothing (body — blue jacket)
    gGfx.setObjectUniforms(mdl, {0.15f,0.25f,0.65f}, 0.85f, 0.0f, 0.f, false, false);
    drawCube({0, 0.5f, 0}, {0.28f,0.48f,0.18f}, {0.15f,0.25f,0.65f});

    // Head (skin)
    gGfx.setObjectUniforms(mdl, {0.85f,0.70f,0.58f}, 0.7f, 0.0f, 0.f, false, false);
    drawCube({0, 1.2f, 0}, {0.19f,0.19f,0.19f}, {0.85f,0.70f,0.58f});

    // Hair
    gGfx.setObjectUniforms(mdl, {0.18f,0.12f,0.07f}, 0.8f, 0.0f, 0.f, false, false);
    drawCube({0, 1.42f, 0}, {0.20f,0.06f,0.20f}, {0.18f,0.12f,0.07f});

    // Trousers
    gGfx.setObjectUniforms(mdl, {0.12f,0.12f,0.40f}, 0.9f, 0.0f, 0.f, false, false);
    drawCube({-0.14f,-0.28f,0}, {0.11f,0.28f,0.11f}, {0.12f,0.12f,0.40f});
    drawCube({ 0.14f,-0.28f,0}, {0.11f,0.28f,0.11f}, {0.12f,0.12f,0.40f});

    // Shoes
    gGfx.setObjectUniforms(mdl, {0.08f,0.08f,0.08f}, 0.6f, 0.3f, 0.f, false, false);
    drawCube({-0.14f,-0.58f, 0.04f}, {0.12f,0.07f,0.15f}, {0.08f,0.08f,0.08f});
    drawCube({ 0.14f,-0.58f, 0.04f}, {0.12f,0.07f,0.15f}, {0.08f,0.08f,0.08f});

    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Original draw primitives (unchanged behaviour)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawCube(Vec3 pos, Vec3 hs, Vec3 color, float alpha) {
    glColor4f(color.x, color.y, color.z, alpha);
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    float x=hs.x, y=hs.y, z=hs.z;
    glBegin(GL_QUADS);
    glNormal3f( 0, 0, 1); glVertex3f(-x,-y, z); glVertex3f( x,-y, z); glVertex3f( x, y, z); glVertex3f(-x, y, z);
    glNormal3f( 0, 0,-1); glVertex3f( x,-y,-z); glVertex3f(-x,-y,-z); glVertex3f(-x, y,-z); glVertex3f( x, y,-z);
    glNormal3f(-1, 0, 0); glVertex3f(-x,-y,-z); glVertex3f(-x,-y, z); glVertex3f(-x, y, z); glVertex3f(-x, y,-z);
    glNormal3f( 1, 0, 0); glVertex3f( x,-y, z); glVertex3f( x,-y,-z); glVertex3f( x, y,-z); glVertex3f( x, y, z);
    glNormal3f( 0, 1, 0); glVertex3f(-x, y, z); glVertex3f( x, y, z); glVertex3f( x, y,-z); glVertex3f(-x, y,-z);
    glNormal3f( 0,-1, 0); glVertex3f(-x,-y,-z); glVertex3f( x,-y,-z); glVertex3f( x,-y, z); glVertex3f(-x,-y, z);
    glEnd();
    glPopMatrix();
}

void Renderer::drawSkyboxLegacy() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    float tod = gfxPanel.timeOfDay;
    float hr = 0.55f + tod*0.12f, hg = 0.72f + tod*0.08f, hb = 0.88f + tod*0.05f;
    float zr = 0.22f + tod*0.15f, zg = 0.42f + tod*0.12f, zb = 0.78f + tod*0.05f;

    glBegin(GL_QUADS);
    glColor3f(hr,hg,hb); glVertex3f(-500,-1,-500); glVertex3f(500,-1,-500);
    glColor3f(zr,zg,zb); glVertex3f(500,200,-500); glVertex3f(-500,200,-500);

    glColor3f(hr,hg,hb); glVertex3f(-500,-1,500); glVertex3f(500,-1,500);
    glColor3f(zr,zg,zb); glVertex3f(500,200,500); glVertex3f(-500,200,500);

    glColor3f(hr,hg,hb); glVertex3f(-500,-1,-500); glVertex3f(-500,-1,500);
    glColor3f(zr,zg,zb); glVertex3f(-500,200,500); glVertex3f(-500,200,-500);

    glColor3f(hr,hg,hb); glVertex3f(500,-1,-500); glVertex3f(500,-1,500);
    glColor3f(zr,zg,zb); glVertex3f(500,200,500); glVertex3f(500,200,-500);
    glEnd();

    // Sun disk
    Vec3 sd = gGfx.getSunDir();
    glColor3f(1.f, 0.92f, 0.6f);
    glPointSize(8.f);
    glBegin(GL_POINTS);
    glVertex3f(sd.x*400, sd.y*400, sd.z*400);
    glEnd();
    glPointSize(1.f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void Renderer::drawGround(float size) {
    glDisable(GL_LIGHTING);
    int tiles = 40; float tileSize = size * 2.f / tiles;
    glBegin(GL_QUADS);
    for (int x = 0; x < tiles; x++)
    for (int z = 0; z < tiles; z++) {
        float wx = -size + x * tileSize;
        float wz = -size + z * tileSize;
        bool road = (x % 5 == 2 || z % 5 == 2);
        if (road) glColor3f(0.24f, 0.24f, 0.26f);
        else      glColor3f(0.30f, 0.32f, 0.28f);
        glVertex3f(wx,0,wz); glVertex3f(wx+tileSize,0,wz);
        glVertex3f(wx+tileSize,0,wz+tileSize); glVertex3f(wx,0,wz+tileSize);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void Renderer::drawBuilding(const Building& b) {
    for (const auto& blk : b.blocks) {
        if (blk.destroyed) continue;
        drawCube(blk.position, blk.body.halfSize, blk.color);
    }
}

void Renderer::drawVehicle(const Vehicle& v) {
    glPushMatrix();
    glTranslatef(v.position.x, v.position.y, v.position.z);
    glRotatef(-v.yaw * 180.f / PI, 0, 1, 0);
    Vec3 hs = v.body.halfSize;
    drawCube({0,0,0}, hs, v.bodyColor);
    Vec3 cab = {hs.x*0.7f, hs.y*0.5f, hs.z*0.5f};
    drawCube({0, hs.y+cab.y, 0}, cab, v.accentColor);
    Vec3 wc = {0.15f,0.15f,0.15f};
    float wy = -hs.y + 0.15f;
    drawCube({-hs.x-0.1f, wy,  hs.z*0.5f}, {0.15f,0.25f,0.35f}, wc);
    drawCube({ hs.x+0.1f, wy,  hs.z*0.5f}, {0.15f,0.25f,0.35f}, wc);
    drawCube({-hs.x-0.1f, wy, -hs.z*0.5f}, {0.15f,0.25f,0.35f}, wc);
    drawCube({ hs.x+0.1f, wy, -hs.z*0.5f}, {0.15f,0.25f,0.35f}, wc);
    if (v.type == VehicleType::BULLDOZER)
        drawCube({0,0,hs.z+0.3f}, {hs.x*1.1f,hs.y*0.8f,0.2f}, {0.7f,0.5f,0.1f});
    glPopMatrix();
}

void Renderer::drawPlayer(const Player& p, const Camera& cam) {
    Vec3 diff = p.position - cam.getEffectivePos();
    if (diff.length() < 3.f) return;
    glPushMatrix();
    glTranslatef(p.position.x, p.position.y, p.position.z);
    glRotatef(-p.yaw * 180.f / PI, 0, 1, 0);
    drawCube({0, 0.5f, 0},  {0.3f, 0.5f, 0.2f}, {0.2f, 0.3f, 0.7f});
    drawCube({0, 1.2f, 0},  {0.2f, 0.2f, 0.2f}, {0.85f,0.7f, 0.6f});
    drawCube({-0.15f,-0.3f,0},{0.12f,0.3f,0.12f},{0.15f,0.15f,0.5f});
    drawCube({ 0.15f,-0.3f,0},{0.12f,0.3f,0.12f},{0.15f,0.15f,0.5f});
    glPopMatrix();
}

void Renderer::drawParticles(const std::vector<ParticleEffect>& particles) {
    glDisable(GL_LIGHTING);
    glPointSize(6.f);
    glBegin(GL_POINTS);
    for (const auto& p : particles) {
        float a = p.life / p.maxLife;
        glColor4f(p.color.x, p.color.y, p.color.z, a);
        glVertex3f(p.position.x, p.position.y, p.position.z);
    }
    glEnd();
    for (const auto& p : particles) {
        if (p.size < 0.2f) continue;
        float a = p.life / p.maxLife;
        drawCube(p.position, Vec3(p.size,p.size,p.size)*0.5f, p.color, a);
    }
    glEnable(GL_LIGHTING);
}

void Renderer::drawExplosionFX(const std::vector<ExplosionForce>& explosions) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    for (const auto& e : explosions) {
        float t = 1.f - e.timeLeft / 0.5f;
        float r = e.radius * (0.3f + t * 0.7f);
        float alpha = (1.f - t) * 0.6f;
        int segs = 16;
        for (int ring = 0; ring < 4; ring++) {
            float ry = sinf((ring/3.f - 0.5f)*PI)*r;
            float rr = cosf((ring/3.f - 0.5f)*PI)*r;
            glColor4f(1.f, 0.4f+ring*0.1f, 0.0f, alpha*(1.f-ring*0.2f));
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < segs; i++) {
                float a = i*2.f*PI/segs;
                glVertex3f(e.origin.x+cosf(a)*rr, e.origin.y+ry, e.origin.z+sinf(a)*rr);
            }
            glEnd();
        }
    }
    glEnable(GL_LIGHTING);
}

void Renderer::drawExplosiveMarkers(const Player& p) {
    glDisable(GL_LIGHTING);
    static sf::Clock blinkClock;
    float t = blinkClock.getElapsedTime().asSeconds();
    for (const auto& e : p.placedExplosives) {
        if (e.detonated) continue;
        float blink = sinf(t*8.f);
        float r = 0.7f + blink*0.3f;
        drawCube(e.position, {0.2f,0.2f,0.2f}, {r,0.1f,0.1f});
        glColor4f(1.f,0.2f,0.2f,0.3f);
        int segs=24;
        glBegin(GL_LINE_LOOP);
        for (int i=0;i<segs;i++) {
            float a=i*2.f*PI/segs;
            glVertex3f(e.position.x+cosf(a)*e.radius,0.1f,e.position.z+sinf(a)*e.radius);
        }
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

void Renderer::drawShockwaves(const std::vector<ShockwaveAnim>& waves) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    for (const auto& sw : waves) {
        float alpha = (sw.life/sw.maxLife)*0.7f;
        int segs = 32;
        glColor4f(sw.color.x,sw.color.y,sw.color.z,alpha);
        glBegin(GL_LINE_LOOP);
        for (int i=0;i<segs;i++) {
            float a=i*2.f*PI/segs;
            glVertex3f(sw.origin.x+cosf(a)*sw.radius,sw.origin.y+0.15f,sw.origin.z+sinf(a)*sw.radius);
        }
        glEnd();
        glColor4f(sw.color.x,sw.color.y*0.6f,0.1f,alpha*0.3f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(sw.origin.x,sw.origin.y+0.1f,sw.origin.z);
        for (int i=0;i<=segs;i++) {
            float a=i*2.f*PI/segs;
            glVertex3f(sw.origin.x+cosf(a)*sw.radius*0.5f,sw.origin.y+0.1f,sw.origin.z+sinf(a)*sw.radius*0.5f);
        }
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

void Renderer::drawDebrisChunks(const std::vector<DebrisAnim>& chunks) {
    glDisable(GL_LIGHTING);
    for (const auto& d : chunks) {
        float a = d.life/d.maxLife;
        glPushMatrix();
        glTranslatef(d.pos.x,d.pos.y,d.pos.z);
        glRotatef(d.rot*180.f/PI, 0.4f,0.7f,0.3f);
        glColor4f(d.color.x,d.color.y,d.color.z,a);
        float s=d.size*0.5f;
        glBegin(GL_QUADS);
        glNormal3f(0,0,1);  glVertex3f(-s,-s,s);  glVertex3f(s,-s,s);  glVertex3f(s,s,s);  glVertex3f(-s,s,s);
        glNormal3f(0,0,-1); glVertex3f(s,-s,-s);  glVertex3f(-s,-s,-s);glVertex3f(-s,s,-s);glVertex3f(s,s,-s);
        glNormal3f(0,1,0);  glVertex3f(-s,s,s);   glVertex3f(s,s,s);   glVertex3f(s,s,-s); glVertex3f(-s,s,-s);
        glNormal3f(0,-1,0); glVertex3f(-s,-s,-s);  glVertex3f(s,-s,-s); glVertex3f(s,-s,s);glVertex3f(-s,-s,s);
        glEnd();
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}
