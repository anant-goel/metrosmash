// ─────────────────────────────────────────────────────────────────────────────
//  Renderer.cpp  —  Metro Smash  —  Complete visual overhaul
//  Fixes: proper sun + sky, explosion FX, lighting, ground, cursor handling
// ─────────────────────────────────────────────────────────────────────────────
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>

#include "Renderer.h"
#include "Camera.h"
#include "GfxShaders.h"
#include "Log.h"

// ─── Init ────────────────────────────────────────────────────────────────────
void Renderer::init(unsigned int w, unsigned int h) {
    screenW = w; screenH = h;
    setupGL();
    gGfx.settings.enableShadows              = false;
    gGfx.settings.enableSSAO                = false;
    gGfx.settings.enableBloom               = false;
    gGfx.settings.timeOfDay                 = gfxPanel.timeOfDay;
    gGfx.init(w, h);
    Log::info("Renderer init %ux%u  gGfx.initialized=%d  fallback=%d",
              w, h, (int)gGfx.initialized, (int)gGfx.fallbackMode);
}

void Renderer::setupGL() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Fixed-function lighting — one warm directional sun
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat matSpec[]   = {0.35f, 0.32f, 0.28f, 1.f};
    GLfloat matShine[]  = {18.f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShine);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glViewport(0, 0, screenW, screenH);
    updateSunLight(gfxPanel.timeOfDay);
}

void Renderer::updateSunLight(float tod) {
    // tod 0=day  0.5=sunset  1=night
    float sunAngle = (1.f - tod) * 70.f + 20.f;  // elevation degrees
    float sunAz    = tod * 60.f;                  // slight azimuth shift at sunset
    float sa = sinf(sunAngle * PI/180.f);
    float ca = cosf(sunAngle * PI/180.f);
    GLfloat lp[] = { ca * cosf(sunAz * PI/180.f) * 500.f,
                     sa * 500.f,
                    -ca * sinf(sunAz * PI/180.f) * 500.f, 0.f };

    // Day=white, sunset=orange, night=deep blue
    float r = tod < 0.5f ? 1.0f : 1.0f - (tod-0.5f)*1.2f;
    float g = tod < 0.5f ? 0.93f: 0.93f - (tod-0.5f)*1.4f;
    float b = tod < 0.5f ? 0.82f: 0.50f - (tod-0.5f)*0.8f;
    r = std::max(0.08f, r); g = std::max(0.05f, g); b = std::max(0.12f, b);

    GLfloat diff[] = {r, g, b, 1.f};
    float am = 0.18f + tod * 0.12f;
    GLfloat amb[]  = {am*0.55f, am*0.60f, am*0.75f, 1.f};
    GLfloat spec[] = {r*0.4f, g*0.4f, b*0.4f, 1.f};

    glLightfv(GL_LIGHT0, GL_POSITION, lp);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
}

void Renderer::resize(unsigned int w, unsigned int h) {
    screenW = w; screenH = h;
    gGfx.resize(w, h);
    glViewport(0, 0, w, h);
}

// ─── Matrix helpers ───────────────────────────────────────────────────────────
void Renderer::setMatrices(const Camera& cam) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glLoadMatrixf(cam.getProjection().m);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLoadMatrixf(cam.getView().m);
}

// ─── Main render ──────────────────────────────────────────────────────────────
void Renderer::render(const World& world, const Camera& camera,
                      const sf::RenderWindow& /*win*/) {
    gameTime += dt;
    updateSunLight(gfxPanel.timeOfDay);
    renderLegacy(world, camera);
}

// ─────────────────────────────────────────────────────────────────────────────
//  LEGACY / ENHANCED  render path
//  Everything is fixed-function GL but with proper lighting, sky, sun, FX
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::renderLegacy(const World& world, const Camera& camera) {
    float tod = gfxPanel.timeOfDay;

    // Sky colour: horizon / zenith
    float hr = 0.72f - tod*0.38f, hg = 0.82f - tod*0.30f, hb = 0.95f - tod*0.20f;
    float zr = 0.28f - tod*0.15f, zg = 0.45f - tod*0.22f, zb = 0.80f - tod*0.10f;
    if (tod > 0.5f) { // sunset/night tint
        float n = (tod - 0.5f) * 2.f;
        hr = 0.34f - n*0.25f;  hg = 0.18f - n*0.12f;  hb = 0.12f - n*0.05f;
        zr = 0.06f - n*0.04f;  zg = 0.05f - n*0.03f;  zb = 0.18f - n*0.10f;
    }
    glClearColor(hr, hg, hb, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setMatrices(camera);

    // ── Sky dome (drawn first, no depth write) ─────────────────────────────
    drawSky(tod, hr, hg, hb, zr, zg, zb, camera);

    // ── Ground ─────────────────────────────────────────────────────────────
    drawGround(tod);

    // ── Buildings (enhanced shading) ───────────────────────────────────────
    for (const auto& b : world.buildings)
        drawBuilding(b, tod);

    // ── Vehicles ───────────────────────────────────────────────────────────
    for (const auto& v : world.vehicles)
        if (!v->destroyed) drawVehicle(*v);

    // ── Player ─────────────────────────────────────────────────────────────
    if (world.player.mode == PlayerMode::ON_FOOT)
        drawPlayer(world.player, camera);

    // ── Vegetation & pedestrians ───────────────────────────────────────────
    static sf::Clock vegClock;
    float vegTime = vegClock.getElapsedTime().asSeconds();
    drawVegetation(world.vegetation, vegTime);
    drawPedestrians(world.pedestrians);

    // ── Explosive markers ──────────────────────────────────────────────────
    drawExplosiveMarkers(world.player);

    // ── Explosion FX (fireballs + shockwaves + debris) ─────────────────────
    drawExplosionFX(world.physics.activeExplosions, world.particles);
    drawShockwaves(world.anim.shockwaves);
    drawDebrisChunks(world.anim.debris);

    // ── Weapon projectiles (meteors, black hole, tornado etc.) ─────────────
    drawActiveWeapons(world.weapons.active);

    // ── Rain ───────────────────────────────────────────────────────────────
    if (gfxPanel.rainEnabled)
        drawRain(camera.getEffectivePos(), tod);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Sky  — atmospheric gradient dome + procedural clouds + sun + moon
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawSky(float tod,
                        float hr, float hg, float hb,
                        float zr, float zg, float zb,
                        const Camera& cam) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    Vec3 eye = cam.getEffectivePos();
    int rings = 10, segs = 24;
    float R = 480.f;

    // Dome with gradient from horizon to zenith
    glBegin(GL_TRIANGLES);
    for (int ring = 0; ring < rings; ring++) {
        float phi0 = (float)ring     / rings * PI * 0.5f;
        float phi1 = (float)(ring+1) / rings * PI * 0.5f;
        float t0 = (float)ring     / rings;
        float t1 = (float)(ring+1) / rings;

        for (int seg = 0; seg < segs; seg++) {
            float th0 = (float)seg     / segs * 2.f * PI;
            float th1 = (float)(seg+1) / segs * 2.f * PI;

            auto skyCol = [&](float t) {
                glColor3f(hr + (zr-hr)*t, hg + (zg-hg)*t, hb + (zb-hb)*t);
            };

            auto vtx = [&](float phi, float th) {
                glVertex3f(eye.x + R*cosf(phi)*cosf(th),
                           eye.y + R*sinf(phi),
                           eye.z + R*cosf(phi)*sinf(th));
            };

            skyCol(t0); vtx(phi0, th0);
            skyCol(t0); vtx(phi0, th1);
            skyCol(t1); vtx(phi1, th0);

            skyCol(t0); vtx(phi0, th1);
            skyCol(t1); vtx(phi1, th1);
            skyCol(t1); vtx(phi1, th0);
        }
    }
    glEnd();

    // ── Sun ──────────────────────────────────────────────────────────────
    {
        float sunAngle  = (1.f - tod) * 70.f + 20.f;
        float sunAz     = tod * 60.f;
        float sa = sinf(sunAngle * PI/180.f);
        float ca = cosf(sunAngle * PI/180.f);
        Vec3 sunDir = {ca*cosf(sunAz*PI/180.f), sa, -ca*sinf(sunAz*PI/180.f)};
        Vec3 sunPos = eye + sunDir * (R * 0.96f);

        // Get camera right/up for billboard
        Vec3 camFwd = cam.getForward().normalized();
        Vec3 right  = Vec3{0,1,0}.cross(camFwd).normalized();
        Vec3 up     = camFwd.cross(right).normalized();

        // Outer glow (soft halo)
        float gr = (tod < 0.5f) ? 1.0f : 0.95f;
        float gg = (tod < 0.5f) ? 0.82f : 0.45f;
        float gb = (tod < 0.5f) ? 0.40f : 0.10f;
        int haloSegs = 32;
        glEnable(GL_BLEND);

        // 3 glow layers
        float glowR[] = {25.f, 15.f, 6.f};
        float glowA[] = {0.08f, 0.18f, 0.85f};
        for (int g = 0; g < 3; g++) {
            float gSize = glowR[g];
            float alpha = glowA[g] * (1.f - tod * 0.6f);
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(gr, gg, gb, alpha);
            glVertex3f(sunPos.x, sunPos.y, sunPos.z);
            glColor4f(gr, gg, gb, 0.f);
            for (int i = 0; i <= haloSegs; i++) {
                float a = i * 2.f * PI / haloSegs;
                Vec3 v = sunPos + right*(cosf(a)*gSize) + up*(sinf(a)*gSize);
                glVertex3f(v.x, v.y, v.z);
            }
            glEnd();
        }

        // Sun disk itself
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.f, tod < 0.5f ? 0.98f : 0.72f, tod < 0.5f ? 0.88f : 0.25f, 1.f);
        glVertex3f(sunPos.x, sunPos.y, sunPos.z);
        glColor4f(gr, gg, gb, 0.9f);
        for (int i = 0; i <= haloSegs; i++) {
            float a = i * 2.f * PI / haloSegs;
            Vec3 v = sunPos + right*(cosf(a)*4.5f) + up*(sinf(a)*4.5f);
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();
    }

    // ── Moon (opposite side, visible at night) ────────────────────────────
    if (tod > 0.4f) {
        float moonAlpha = (tod - 0.4f) / 0.6f;
        Vec3 moonPos = eye + Vec3{-200.f, 280.f, -350.f};
        Vec3 camFwd  = cam.getForward().normalized();
        Vec3 right   = Vec3{0,1,0}.cross(camFwd).normalized();
        Vec3 up      = camFwd.cross(right).normalized();

        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.92f, 0.94f, 0.98f, moonAlpha);
        glVertex3f(moonPos.x, moonPos.y, moonPos.z);
        glColor4f(0.75f, 0.80f, 0.88f, 0.f);
        for (int i = 0; i <= 20; i++) {
            float a = i * 2.f * PI / 20;
            Vec3 v = moonPos + right*(cosf(a)*8.f) + up*(sinf(a)*8.f);
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();
    }

    // ── Procedural clouds ─────────────────────────────────────────────────
    if (tod < 0.7f) {
        float cloudAlpha = (1.f - tod * 1.2f) * 0.55f;
        struct Cloud { float x, z, size, density; };
        static std::vector<Cloud> clouds;
        if (clouds.empty()) {
            for (int i = 0; i < 18; i++)
                clouds.push_back({ (float)(rand()%800-400),
                                   (float)(rand()%600-300),
                                   (float)(40+rand()%80),
                                   0.35f + (rand()%6)*0.1f });
        }
        float cy = eye.y + 140.f;
        for (auto& cl : clouds) {
            cl.x += 0.6f * dt; // drift
            if (cl.x > 450.f) cl.x = -450.f;
            // Puffy cloud — 3 overlapping ellipses
            for (int b = 0; b < 3; b++) {
                float bx = cl.x + eye.x + b*cl.size*0.35f - cl.size*0.35f;
                float bz = cl.z + eye.z + b*5.f;
                float bs = cl.size * (0.6f + b*0.25f);
                float cr2 = 0.92f, cg2 = 0.93f, cb2 = 0.97f;
                glBegin(GL_TRIANGLE_FAN);
                glColor4f(cr2, cg2, cb2, cloudAlpha * cl.density);
                glVertex3f(bx, cy, bz);
                glColor4f(cr2, cg2, cb2, 0.f);
                int csegs = 14;
                for (int i = 0; i <= csegs; i++) {
                    float a = i * 2.f * PI / csegs;
                    glVertex3f(bx + cosf(a)*bs, cy + sinf(a)*bs*0.35f,
                               bz + sinf(a)*bs * 0.7f);
                }
                glEnd();
            }
        }
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ground — tiled with roads, pavements, grass patches, road markings
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawGround(float tod) {
    glDisable(GL_LIGHTING);

    float nightDim = 1.f - tod * 0.45f;
    int tiles = 80;
    float sz  = 300.f;
    float tSz = sz * 2.f / tiles;

    glBegin(GL_QUADS);
    for (int x = 0; x < tiles; x++)
    for (int z = 0; z < tiles; z++) {
        float wx = -sz + x * tSz;
        float wz = -sz + z * tSz;

        bool roadX  = (x % 5 == 2);
        bool roadZ  = (z % 5 == 2);
        bool road   = roadX || roadZ;
        bool curb   = !road && ((x%5==1)||(x%5==3)||(z%5==1)||(z%5==3));
        bool center = roadX && roadZ;

        Vec3 c;
        if      (center) c = {0.30f, 0.30f, 0.32f};  // intersection
        else if (road)   c = {0.22f, 0.22f, 0.24f};  // tarmac
        else if (curb)   c = {0.52f, 0.52f, 0.50f};  // pavement/sidewalk
        else {
            // Grass — slight variation
            float g = 0.27f + ((x*7+z*13)%8)*0.015f;
            c = {0.18f, g, 0.14f};
        }
        c.x *= nightDim; c.y *= nightDim; c.z *= nightDim;
        glColor3f(c.x, c.y, c.z);
        glNormal3f(0,1,0);
        glVertex3f(wx,      0, wz);
        glVertex3f(wx+tSz,  0, wz);
        glVertex3f(wx+tSz,  0, wz+tSz);
        glVertex3f(wx,      0, wz+tSz);
    }
    glEnd();

    // Road centre-line dashes
    glDisable(GL_DEPTH_TEST);
    glColor4f(0.9f*nightDim, 0.85f*nightDim, 0.1f*nightDim, 0.9f);
    glLineWidth(2.f);
    glBegin(GL_LINES);
    for (int x = 0; x < tiles; x++) {
        bool roadX = (x % 5 == 2);
        if (!roadX) continue;
        float wx = -sz + x * tSz + tSz * 0.5f;
        for (int z = 0; z < tiles; z += 2) {
            float wz = -sz + z * tSz;
            glVertex3f(wx, 0.02f, wz);
            glVertex3f(wx, 0.02f, wz + tSz * 0.8f);
        }
    }
    for (int z = 0; z < tiles; z++) {
        bool roadZ = (z % 5 == 2);
        if (!roadZ) continue;
        float wz = -sz + z * tSz + tSz * 0.5f;
        for (int x = 0; x < tiles; x += 2) {
            float wx = -sz + x * tSz;
            glVertex3f(wx,          0.02f, wz);
            glVertex3f(wx + tSz*0.8f, 0.02f, wz);
        }
    }
    glEnd();
    glLineWidth(1.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Building — enhanced: window glow, crack darkening, block outlines
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawBuilding(const Building& b, float tod) {
    float nightDim = 1.f - tod * 0.3f;

    for (const auto& blk : b.blocks) {
        if (blk.destroyed) continue;

        Vec3 col = blk.color;
        col.x *= nightDim; col.y *= nightDim; col.z *= nightDim;

        // Crack darkening
        if (blk.crackLevel > 0) {
            float darken = 1.f - blk.crackLevel * 0.18f;
            col.x *= darken; col.y *= darken; col.z *= darken;
        }

        drawCube(blk.position, blk.body.halfSize, col);

        // Window night glow — warm yellow light emanating from glass blocks
        if (blk.type == 1 && tod > 0.3f) {
            float glow = (tod - 0.3f) * 1.4f;
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            // Halo around window
            Vec3 wPos = blk.position;
            Vec3 wHs  = blk.body.halfSize;
            glColor4f(1.0f, 0.85f, 0.45f, glow * 0.25f);
            drawCube(wPos, wHs * 1.15f, {1,0.85f,0.45f}, glow * 0.2f);
            glEnable(GL_LIGHTING);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Vehicle
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawVehicle(const Vehicle& v) {
    glPushMatrix();
    glTranslatef(v.position.x, v.position.y, v.position.z);
    glRotatef(-v.yaw * 180.f / PI, 0, 1, 0);
    Vec3 hs = v.body.halfSize;

    // Body
    drawCube({0,0,0}, hs, v.bodyColor);

    // Cab / roof
    Vec3 cab = {hs.x*0.68f, hs.y*0.52f, hs.z*0.55f};
    drawCube({0, hs.y+cab.y, -hs.z*0.1f}, cab, v.accentColor);

    // Windscreen (semi-transparent blue)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glColor4f(0.45f, 0.65f, 0.85f, 0.45f);
    float wx=hs.x*0.60f, wz=0.04f, wy=hs.y*0.38f;
    float frontZ = hs.z + 0.04f;
    glBegin(GL_QUADS);
    glNormal3f(0,0,1);
    glVertex3f(-wx, hs.y*0.1f,  frontZ);
    glVertex3f( wx, hs.y*0.1f,  frontZ);
    glVertex3f( wx, hs.y*0.1f + wy*2, frontZ);
    glVertex3f(-wx, hs.y*0.1f + wy*2, frontZ);
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    // Headlights
    glDisable(GL_LIGHTING);
    glColor3f(1.f, 0.97f, 0.82f);
    drawCube({-hs.x*0.62f, -hs.y*0.15f, hs.z+0.06f}, {0.14f,0.09f,0.04f}, {1,0.97f,0.82f});
    drawCube({ hs.x*0.62f, -hs.y*0.15f, hs.z+0.06f}, {0.14f,0.09f,0.04f}, {1,0.97f,0.82f});
    // Tail lights
    glColor3f(0.9f, 0.1f, 0.1f);
    drawCube({-hs.x*0.62f, -hs.y*0.15f, -hs.z-0.06f}, {0.14f,0.09f,0.04f}, {0.9f,0.1f,0.1f});
    drawCube({ hs.x*0.62f, -hs.y*0.15f, -hs.z-0.06f}, {0.14f,0.09f,0.04f}, {0.9f,0.1f,0.1f});
    glEnable(GL_LIGHTING);

    // Wheels (dark rubber with grey hubcap)
    float wy2 = -hs.y + 0.12f;
    Vec3 wheelCol = {0.12f, 0.12f, 0.14f};
    Vec3 hubCol   = {0.55f, 0.55f, 0.58f};
    for (int side : {-1, 1}) {
        for (int fwd : {-1, 1}) {
            Vec3 wp = {side*(hs.x+0.13f), wy2, fwd*hs.z*0.52f};
            drawCube(wp, {0.13f, 0.28f, 0.30f}, wheelCol);
            drawCube(wp + Vec3{(float)side*0.14f,0,0}, {0.04f,0.20f,0.22f}, hubCol);
        }
    }
    if (v.type == VehicleType::BULLDOZER)
        drawCube({0, 0, hs.z+0.32f}, {hs.x*1.15f, hs.y*0.85f, 0.22f}, {0.72f,0.52f,0.12f});
    if (v.type == VehicleType::TANK) {
        drawCube({0, hs.y+0.18f, 0}, {hs.x*0.55f, 0.22f, hs.z*0.55f}, {0.35f,0.40f,0.28f});
        drawCube({0, hs.y+0.4f,  hs.z*0.4f}, {0.12f, 0.12f, hs.z*0.6f}, {0.28f,0.32f,0.22f});
    }
    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Player — 3D articulated character
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawPlayer(const Player& p, const Camera& cam) {
    if ((p.position - cam.getEffectivePos()).length() < 2.5f) return;
    glPushMatrix();
    glTranslatef(p.position.x, p.position.y, p.position.z);
    glRotatef(-p.yaw * 180.f / PI, 0, 1, 0);

    // Torso
    drawCube({0, 0.48f, 0},    {0.27f, 0.45f, 0.16f}, {0.15f, 0.28f, 0.72f});
    // Head
    drawCube({0, 1.18f, 0},    {0.18f, 0.18f, 0.18f}, {0.87f, 0.72f, 0.58f});
    // Hair
    drawCube({0, 1.38f, -0.02f},{0.19f, 0.06f, 0.19f},{0.18f, 0.12f, 0.07f});
    // Left arm
    drawCube({-0.35f, 0.42f, 0},{0.09f, 0.38f, 0.09f},{0.87f, 0.72f, 0.58f});
    // Right arm
    drawCube({ 0.35f, 0.42f, 0},{0.09f, 0.38f, 0.09f},{0.87f, 0.72f, 0.58f});
    // Left leg
    drawCube({-0.13f,-0.32f, 0},{0.10f, 0.32f, 0.10f},{0.12f, 0.14f, 0.42f});
    // Right leg
    drawCube({ 0.13f,-0.32f, 0},{0.10f, 0.32f, 0.10f},{0.12f, 0.14f, 0.42f});
    // Shoes
    drawCube({-0.13f,-0.64f, 0.04f},{0.11f, 0.06f, 0.14f},{0.08f,0.08f,0.08f});
    drawCube({ 0.13f,-0.64f, 0.04f},{0.11f, 0.06f, 0.14f},{0.08f,0.08f,0.08f});

    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Cube primitive
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawCube(Vec3 pos, Vec3 hs, Vec3 col, float alpha) {
    glColor4f(col.x, col.y, col.z, alpha);
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    float x=hs.x, y=hs.y, z=hs.z;
    glBegin(GL_QUADS);
    glNormal3f( 0, 0, 1); glVertex3f(-x,-y,z); glVertex3f(x,-y,z); glVertex3f(x,y,z); glVertex3f(-x,y,z);
    glNormal3f( 0, 0,-1); glVertex3f(x,-y,-z); glVertex3f(-x,-y,-z); glVertex3f(-x,y,-z); glVertex3f(x,y,-z);
    glNormal3f(-1, 0, 0); glVertex3f(-x,-y,-z); glVertex3f(-x,-y,z); glVertex3f(-x,y,z); glVertex3f(-x,y,-z);
    glNormal3f( 1, 0, 0); glVertex3f(x,-y,z); glVertex3f(x,-y,-z); glVertex3f(x,y,-z); glVertex3f(x,y,z);
    glNormal3f( 0, 1, 0); glVertex3f(-x,y,z); glVertex3f(x,y,z); glVertex3f(x,y,-z); glVertex3f(-x,y,-z);
    glNormal3f( 0,-1, 0); glVertex3f(-x,-y,-z); glVertex3f(x,-y,-z); glVertex3f(x,-y,z); glVertex3f(-x,-y,z);
    glEnd();
    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
//  EXPLOSION FX  —  massive billboarded fireballs + shockring + smoke
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawExplosionFX(const std::vector<ExplosionForce>& explosions,
                                const std::vector<ParticleEffect>& particles) {
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive for fire glow
    glDepthMask(GL_FALSE);

    for (const auto& e : explosions) {
        float life  = e.timeLeft;
        float total = 0.5f;
        float t     = 1.f - life / total;       // 0=fresh  1=old
        float r     = e.radius * (0.2f + t * 0.9f);
        float alpha = life / total;

        // ── Core fireball ──────────────────────────────────────────────
        int segs = 24;
        // Multiple overlapping rings of fire
        for (int layer = 0; layer < 5; layer++) {
            float lr   = r * (0.5f + layer * 0.18f);
            float la   = alpha * (1.f - layer * 0.18f);
            float ry   = e.origin.y + r * (layer * 0.12f - 0.1f);

            // Orange/yellow core fading to red
            float fr = 1.0f;
            float fg = layer < 2 ? 0.7f - layer*0.2f : 0.1f;
            float fb = 0.0f;

            glBegin(GL_TRIANGLE_FAN);
            glColor4f(fr, fg, fb, la * 0.9f);
            glVertex3f(e.origin.x, ry, e.origin.z);
            glColor4f(fr*0.6f, fg*0.3f, 0.f, 0.f);
            for (int i = 0; i <= segs; i++) {
                float a = i * 2.f * PI / segs;
                glVertex3f(e.origin.x + cosf(a)*lr,
                           ry + sinf(a)*lr*0.55f,
                           e.origin.z + sinf(a)*lr);
            }
            glEnd();
        }

        // ── Vertical pillar ────────────────────────────────────────────
        if (t < 0.5f) {
            float pAlpha = alpha * 0.7f * (1.f - t*2.f);
            float ph     = r * 1.5f;
            glBegin(GL_QUADS);
            glColor4f(1.f, 0.5f, 0.05f, pAlpha);
            glVertex3f(e.origin.x - r*0.3f, e.origin.y,    e.origin.z);
            glVertex3f(e.origin.x + r*0.3f, e.origin.y,    e.origin.z);
            glColor4f(0.7f, 0.15f, 0.f, 0.f);
            glVertex3f(e.origin.x + r*0.15f, e.origin.y+ph, e.origin.z);
            glVertex3f(e.origin.x - r*0.15f, e.origin.y+ph, e.origin.z);
            glEnd();
        }

        // ── Mushroom cap (large blasts) ─────────────────────────────────
        if (e.radius > 8.f && t > 0.1f && t < 0.7f) {
            float capR   = r * 0.9f;
            float capY   = e.origin.y + r * 1.2f;
            float capAlp = alpha * 0.5f;
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.85f, 0.45f, 0.1f, capAlp);
            glVertex3f(e.origin.x, capY, e.origin.z);
            glColor4f(0.5f, 0.2f, 0.05f, 0.f);
            for (int i = 0; i <= segs; i++) {
                float a = i * 2.f * PI / segs;
                glVertex3f(e.origin.x + cosf(a)*capR,
                           capY - sinf(a)*capR*0.3f,
                           e.origin.z + sinf(a)*capR);
            }
            glEnd();
        }

        // ── Ground shockring ───────────────────────────────────────────
        float sAlpha = alpha * 0.55f;
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.f, 0.7f, 0.3f, sAlpha * 0.8f);
        glVertex3f(e.origin.x, 0.05f, e.origin.z);
        glColor4f(0.8f, 0.3f, 0.0f, 0.f);
        for (int i = 0; i <= segs; i++) {
            float a = i * 2.f * PI / segs;
            glVertex3f(e.origin.x + cosf(a)*r, 0.05f, e.origin.z + sinf(a)*r);
        }
        glEnd();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        // ── Smoke column ───────────────────────────────────────────────
        if (t > 0.3f) {
            float smokeT = (t - 0.3f) / 0.7f;
            float smokeR = r * 0.7f * smokeT;
            float smokeY = e.origin.y + r * smokeT * 2.5f;
            float smokeA = (1.f - smokeT) * alpha * 0.4f;
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.22f, 0.20f, 0.18f, smokeA);
            glVertex3f(e.origin.x, smokeY, e.origin.z);
            glColor4f(0.15f, 0.14f, 0.12f, 0.f);
            for (int i = 0; i <= segs; i++) {
                float a = i * 2.f * PI / segs;
                glVertex3f(e.origin.x + cosf(a)*smokeR,
                           smokeY + sinf(a)*smokeR*0.3f,
                           e.origin.z + sinf(a)*smokeR);
            }
            glEnd();
        }
    }

    // ── Particle FX (fire + debris dots) ─────────────────────────────────
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(5.f);
    glBegin(GL_POINTS);
    for (const auto& p : particles) {
        float a = std::max(0.f, p.life / p.maxLife);
        glColor4f(p.color.x, p.color.y, p.color.z, a);
        glVertex3f(p.position.x, p.position.y, p.position.z);
    }
    glEnd();

    // Larger particle cubes
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (const auto& p : particles) {
        if (p.size < 0.15f) continue;
        float a = std::max(0.f, p.life / p.maxLife);
        drawCube(p.position, Vec3{p.size,p.size,p.size}*0.5f, p.color, a);
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Active weapon projectile visuals
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawActiveWeapons(const std::vector<ActiveWeapon>& active) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    for (const auto& w : active) {
        if (w.detonated) continue;
        Vec3 p = w.position;

        switch (w.type) {
        case WeaponType::METEOR_SHOWER: {
            // Glowing orange meteor with fiery trail
            float sz = 1.4f;
            glDisable(GL_CULL_FACE);
            drawCube(p, {sz,sz,sz}, {1.f,0.45f,0.05f}, 0.9f);
            // Trail
            for (int t = 1; t <= 6; t++) {
                float ta = 0.6f - t*0.09f;
                Vec3 tp = p + Vec3{0, t*1.8f, 0};
                drawCube(tp, Vec3{sz*0.6f,sz*0.6f,sz*0.6f}*(1.f-t*0.12f),
                         {1.f, 0.3f-t*0.04f, 0.f}, ta);
            }
            glEnable(GL_CULL_FACE);
            break;
        }
        case WeaponType::BLACK_HOLE: {
            // Pulsing purple/dark sphere
            static float bht = 0.f; bht += dt;
            float pulse = 0.7f + 0.3f * sinf(bht * 8.f);
            int segs = 20;
            float r = 3.5f * pulse;
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.4f, 0.0f, 0.6f, 0.95f);
            glVertex3f(p.x, p.y, p.z);
            glColor4f(0.1f, 0.0f, 0.2f, 0.f);
            for (int i = 0; i <= segs; i++) {
                float a = i * 2.f * PI / segs;
                glVertex3f(p.x+cosf(a)*r, p.y+sinf(a)*r*0.5f, p.z+sinf(a)*r);
            }
            glEnd();
            // Accretion ring
            glBegin(GL_LINE_LOOP);
            glColor4f(0.8f, 0.3f, 1.f, 0.9f);
            for (int i = 0; i < segs; i++) {
                float a = i * 2.f * PI / segs + bht*3.f;
                glVertex3f(p.x+cosf(a)*r*1.6f, p.y, p.z+sinf(a)*r*1.6f);
            }
            glEnd();
            break;
        }
        case WeaponType::TORNADO: {
            // Rotating funnel
            static float tnt = 0.f; tnt += dt;
            int rings = 8;
            for (int ri = 0; ri < rings; ri++) {
                float frac = (float)ri / rings;
                float rr   = 1.5f + frac * 6.f;
                float ry   = p.y + frac * 20.f;
                float rot  = tnt * 4.f - frac * 2.f;
                int segs2  = 12;
                float alpha2 = 0.4f - frac * 0.04f;
                glBegin(GL_LINE_LOOP);
                glColor4f(0.7f, 0.8f, 0.9f, alpha2);
                for (int i = 0; i < segs2; i++) {
                    float a = i * 2.f * PI / segs2 + rot;
                    glVertex3f(p.x+cosf(a)*rr, ry, p.z+sinf(a)*rr);
                }
                glEnd();
            }
            break;
        }
        case WeaponType::WORMHOLE: {
            // Spinning portal ring
            static float wht = 0.f; wht += dt;
            int segs3 = 24;
            for (int ring2 = 0; ring2 < 3; ring2++) {
                float r2 = 4.f + ring2 * 1.5f;
                float rot2 = wht * (2.f + ring2);
                glBegin(GL_LINE_LOOP);
                glColor4f(0.f, 0.8f + ring2*0.07f, 1.f, 0.7f - ring2*0.2f);
                for (int i = 0; i < segs3; i++) {
                    float a = i * 2.f * PI / segs3 + rot2;
                    glVertex3f(p.x+cosf(a)*r2, p.y+sinf(a)*r2, p.z);
                }
                glEnd();
            }
            break;
        }
        case WeaponType::LIGHTNING_STORM: {
            // Crackling bolt from sky
            glColor4f(0.7f, 0.85f, 1.f, 0.85f);
            glLineWidth(3.f);
            glBegin(GL_LINE_STRIP);
            float bx = p.x, bz = p.z;
            for (int seg2 = 0; seg2 < 10; seg2++) {
                float by = p.y + (10 - seg2) * 6.f;
                glVertex3f(bx, by, bz);
                bx += (float)(rand()%6-3);
                bz += (float)(rand()%6-3);
            }
            glVertex3f(p.x, p.y, p.z);
            glEnd();
            glLineWidth(1.f);
            break;
        }
        case WeaponType::VOLCANO: {
            // Lava blob — glowing orange cube
            drawCube(p, {0.6f,0.6f,0.6f}, {1.f, 0.35f, 0.0f}, 0.9f);
            break;
        }
        default:
            // Generic projectile dot
            glPointSize(8.f);
            glBegin(GL_POINTS);
            glColor4f(1.f, 0.8f, 0.2f, 0.9f);
            glVertex3f(p.x, p.y, p.z);
            glEnd();
            glPointSize(1.f);
            break;
        }
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Shockwave rings
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawShockwaves(const std::vector<ShockwaveAnim>& waves) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (const auto& sw : waves) {
        float alpha = (sw.life / sw.maxLife) * 0.8f;
        int segs = 40;

        // Outer ring
        glLineWidth(3.f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segs; i++) {
            float a = i * 2.f * PI / segs;
            float fa = alpha * (0.5f + 0.5f * sinf(a * 3.f + sw.life * 10.f));
            glColor4f(sw.color.x, sw.color.y*0.6f, 0.1f, fa);
            glVertex3f(sw.origin.x + cosf(a)*sw.radius,
                       sw.origin.y + 0.2f,
                       sw.origin.z + sinf(a)*sw.radius);
        }
        glEnd();
        glLineWidth(1.f);

        // Filled shockwave disc
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(sw.color.x, sw.color.y*0.5f, 0.05f, alpha * 0.3f);
        glVertex3f(sw.origin.x, sw.origin.y+0.1f, sw.origin.z);
        glColor4f(sw.color.x, sw.color.y*0.3f, 0.f, 0.f);
        for (int i = 0; i <= segs; i++) {
            float a = i * 2.f * PI / segs;
            glVertex3f(sw.origin.x + cosf(a)*sw.radius,
                       sw.origin.y+0.1f,
                       sw.origin.z + sinf(a)*sw.radius);
        }
        glEnd();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Debris chunks
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawDebrisChunks(const std::vector<DebrisAnim>& chunks) {
    for (const auto& d : chunks) {
        float a = d.life / d.maxLife;
        glPushMatrix();
        glTranslatef(d.pos.x, d.pos.y, d.pos.z);
        glRotatef(d.rot * 180.f / PI, 0.4f, 0.7f, 0.3f);
        drawCube({0,0,0}, {d.size,d.size,d.size}*0.5f, d.color, a);
        glPopMatrix();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Explosive markers (C4 blinkers)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawExplosiveMarkers(const Player& p) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    static sf::Clock blinkClock;
    float t = blinkClock.getElapsedTime().asSeconds();
    for (const auto& e : p.placedExplosives) {
        if (e.detonated) continue;
        float blink = 0.6f + 0.4f * sinf(t * 8.f);
        drawCube(e.position, {0.2f,0.2f,0.2f}, {blink, 0.05f, 0.05f});
        glColor4f(1.f, 0.1f, 0.1f, 0.25f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(e.position.x, 0.08f, e.position.z);
        glColor4f(1.f,0.1f,0.1f, 0.f);
        for (int i = 0; i <= 24; i++) {
            float a = i * 2.f * PI / 24;
            glVertex3f(e.position.x + cosf(a)*e.radius,
                       0.08f,
                       e.position.z + sinf(a)*e.radius);
        }
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Rain
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawRain(Vec3 cam, float tod) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float nightBright = 1.f - tod * 0.5f;
    glColor4f(0.65f*nightBright, 0.72f*nightBright, 0.88f*nightBright, 0.30f);
    glLineWidth(1.2f);

    static std::vector<Vec3> drops;
    if (drops.empty()) {
        drops.resize(2000);
        for (auto& d : drops)
            d = {cam.x+(float)(rand()%160-80), (float)(rand()%30),
                 cam.z+(float)(rand()%160-80)};
    }
    glBegin(GL_LINES);
    for (auto& d : drops) {
        d.y -= 30.f * dt;
        d.x -= 1.2f * dt;
        if (d.y < 0.f) {
            d.y = 28.f + (float)(rand()%10);
            d.x = cam.x + (float)(rand()%160-80);
            d.z = cam.z + (float)(rand()%160-80);
        }
        glVertex3f(d.x, d.y, d.z);
        glVertex3f(d.x+0.25f, d.y-0.8f, d.z);
    }
    glEnd();
    glLineWidth(1.f);
    glEnable(GL_LIGHTING);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Vegetation — trees with proper 3-layer conical canopy + sway
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawVegetation(const std::vector<VegetationNode>& nodes, float time) {
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    for (const auto& n : nodes) {
        glPushMatrix();
        glTranslatef(n.position.x, 0.f, n.position.z);

        if (n.kind == VegetationNode::Kind::LAMP) {
            glDisable(GL_LIGHTING);
            // Pole
            glColor3f(0.28f, 0.28f, 0.32f);
            drawCube({0, n.height*0.5f, 0}, {0.07f, n.height*0.5f, 0.07f},
                     {0.28f, 0.28f, 0.32f});
            // Arm
            glBegin(GL_LINES);
            glVertex3f(0, n.height, 0); glVertex3f(0.8f, n.height+0.25f, 0);
            glEnd();
            // Bulb
            static sf::Clock lc;
            float flicker = 0.92f + 0.08f * sinf(lc.getElapsedTime().asSeconds()*47.f + n.swayPhase);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(n.leafColor.x*flicker, n.leafColor.y*flicker,
                      n.leafColor.z*flicker, 0.9f);
            glVertex3f(0.8f, n.height+0.25f, 0.f);
            glColor4f(n.leafColor.x, n.leafColor.y, n.leafColor.z, 0.f);
            for (int i = 0; i <= 12; i++) {
                float a = i * 2.f * PI / 12;
                glVertex3f(0.8f+cosf(a)*0.4f, n.height+0.25f+sinf(a)*0.4f, 0.f);
            }
            glEnd();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LIGHTING);
        }
        else {
            float sway  = n.swayAmp * sinf(time * 1.3f + n.swayPhase) *
                          (1.f + 0.4f * sinf(time * 2.8f + n.swayPhase * 0.7f));
            float swayZ = n.swayAmp * 0.6f * cosf(time * 1.1f + n.swayPhase + 1.f);
            glRotatef(sway  * 180.f / PI, 0, 0, 1);
            glRotatef(swayZ * 180.f / PI, 1, 0, 0);

            if (n.kind == VegetationNode::Kind::TREE) {
                // Trunk
                Vec3 tc = n.burning ? Vec3{0.35f,0.18f,0.04f} : Vec3{0.42f,0.28f,0.12f};
                glEnable(GL_LIGHTING);
                drawCube({0, n.height*0.4f, 0},
                         {0.16f+n.height*0.022f, n.height*0.4f, 0.16f+n.height*0.022f},
                         tc);

                if (!n.burning) {
                    // 3-layer conical canopy
                    glDisable(GL_LIGHTING);
                    for (int L = 0; L < 3; L++) {
                        float frac = (float)L / 3.f;
                        float cy   = n.height * (0.48f + frac * 0.42f);
                        float cr   = n.radius  * (1.f - frac * 0.52f);
                        float ch   = n.height  * 0.36f;
                        float bright = 1.f - frac * 0.22f;
                        Vec3  lc = n.leafColor;
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glColor4f(lc.x*bright, lc.y*bright, lc.z*bright, 0.93f);
                        int segs = 14;
                        glBegin(GL_TRIANGLE_FAN);
                        glVertex3f(0, cy+ch, 0);
                        for (int i = 0; i <= segs; i++) {
                            float a = i * 2.f * PI / segs;
                            glVertex3f(cosf(a)*cr, cy, sinf(a)*cr);
                        }
                        glEnd();
                        glBegin(GL_TRIANGLE_FAN);
                        glVertex3f(0, cy, 0);
                        for (int i = segs; i >= 0; i--) {
                            float a = i * 2.f * PI / segs;
                            glVertex3f(cosf(a)*cr, cy, sinf(a)*cr);
                        }
                        glEnd();
                    }
                    glEnable(GL_LIGHTING);
                } else {
                    // Fire blob
                    glDisable(GL_LIGHTING);
                    static sf::Clock fc;
                    float fl = 0.7f+0.3f*sinf(fc.getElapsedTime().asSeconds()*12.f+n.swayPhase);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    glColor4f(1.f, 0.35f*fl, 0.f, 0.7f);
                    glBegin(GL_TRIANGLE_FAN);
                    glVertex3f(0, n.height+n.radius, 0);
                    for (int i = 0; i <= 12; i++) {
                        float a = i * 2.f * PI / 12;
                        glVertex3f(cosf(a)*n.radius, n.height*0.55f, sinf(a)*n.radius);
                    }
                    glEnd();
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glEnable(GL_LIGHTING);
                }
            } else { // BUSH
                Vec3 bc = n.burning ? Vec3{0.6f,0.2f,0.f} : n.leafColor;
                glDisable(GL_LIGHTING);
                glColor4f(bc.x, bc.y, bc.z, 0.88f);
                // Hemisphere top
                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(0, n.height, 0);
                for (int i = 0; i <= 12; i++) {
                    float a = i * 2.f * PI / 12;
                    glVertex3f(cosf(a)*n.radius, n.height*0.5f, sinf(a)*n.radius);
                }
                glEnd();
                // Skirt
                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(0, 0.05f, 0);
                for (int i = 12; i >= 0; i--) {
                    float a = i * 2.f * PI / 12;
                    glVertex3f(cosf(a)*n.radius, n.height*0.42f, sinf(a)*n.radius);
                }
                glEnd();
                glEnable(GL_LIGHTING);
            }
        }
        glPopMatrix();
    }

    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pedestrians
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawPedestrians(const std::vector<Pedestrian>& peds) {
    for (const auto& p : peds) {
        glPushMatrix();
        glTranslatef(p.position.x, 0, p.position.z);
        glRotatef(-p.yaw * 180.f / PI, 0, 1, 0);
        // Colour based on speed (unique per person)
        Vec3 bodyCol = p.panicking ? Vec3{0.8f,0.1f,0.1f}
                                   : Vec3{0.3f+p.speed*0.08f, 0.35f, 0.6f-p.speed*0.05f};
        drawCube({0, 0.55f, 0}, {0.12f, 0.25f, 0.10f}, bodyCol);
        drawCube({0, 1.00f, 0}, {0.10f, 0.10f, 0.10f}, {0.85f, 0.68f, 0.50f});
        glPopMatrix();
    }
}

// ─── Stubs (advanced path not active — keep linker satisfied) ─────────────────
void Renderer::renderAdvanced(const World&, const Camera&) {}
void Renderer::renderShadowPass(const World&, const Camera&) {}
void Renderer::drawBuildingShadow(const Building&) {}
void Renderer::drawGroundAdvanced() {}
void Renderer::drawBuildingAdvanced(const Building&) {}
void Renderer::drawVehicleAdvanced(const Vehicle&) {}
void Renderer::drawPlayerAdvanced(const Player&, const Camera&) {}
void Renderer::drawSkyboxLegacy() {}
void Renderer::drawParticles(const std::vector<ParticleEffect>&) {}
void Renderer::drawExplosionFX(const std::vector<ExplosionForce>&) {}
void Renderer::drawCubeInstanced(Vec3,Vec3,Vec3,float,float,float,bool,bool) {}
