#include "Renderer.h"
#include "Camera.h"
#include <SFML/OpenGL.hpp>
#include <cmath>

void Renderer::init(unsigned int width, unsigned int height) {
    screenW = width;
    screenH = height;
    setupGL();
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

    // Light setup
    GLfloat lightPos[]  = {50.f, 100.f, 50.f, 1.f};
    GLfloat lightDiff[] = {1.0f,  0.95f, 0.85f, 1.f};
    GLfloat lightAmb[]  = {0.35f, 0.35f, 0.40f, 1.f};
    GLfloat lightSpec[] = {0.5f,  0.5f,  0.5f,  1.f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiff);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);

    glViewport(0, 0, screenW, screenH);
}

void Renderer::resize(unsigned int w, unsigned int h) {
    screenW = w; screenH = h;
    glViewport(0, 0, w, h);
}

void Renderer::setMatrices(const Camera& cam) {
    // Projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const Mat4& proj = cam.getProjection();
    glLoadMatrixf(proj.m);

    // View
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const Mat4& view = cam.getView();
    glLoadMatrixf(view.m);
}

void Renderer::render(const World& world, const Camera& camera,
                      const sf::RenderWindow& /*win*/) {
    // Sky gradient clear
    glClearColor(0.45f, 0.65f, 0.85f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setMatrices(camera);

    drawSkybox();
    drawGround(250.f);

    // Buildings
    for (const auto& b : world.buildings) {
        drawBuilding(b);
    }

    // Vehicles
    for (const auto& v : world.vehicles) {
        if (!v->destroyed) drawVehicle(*v);
    }

    // Player (when not in vehicle and not 1st person)
    if (world.player.mode == PlayerMode::ON_FOOT) {
        drawPlayer(world.player, camera);
    }

    // Particles and FX
    drawParticles(world.particles);
    drawExplosionFX(world.physics.activeExplosions);
    drawExplosiveMarkers(world.player);
}

void Renderer::drawSkybox() {
    // Simple gradient sky using a large sphere-like approach via quads
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glBegin(GL_QUADS);
    // Horizon
    glColor3f(0.6f, 0.75f, 0.9f);
    glVertex3f(-500,-1,-500); glVertex3f(500,-1,-500);
    glColor3f(0.3f, 0.5f, 0.8f);
    glVertex3f(500, 200,-500); glVertex3f(-500,200,-500);

    glColor3f(0.6f, 0.75f, 0.9f);
    glVertex3f(-500,-1,500); glVertex3f(500,-1,500);
    glColor3f(0.3f, 0.5f, 0.8f);
    glVertex3f(500, 200,500); glVertex3f(-500,200,500);

    glColor3f(0.6f, 0.75f, 0.9f);
    glVertex3f(-500,-1,-500); glVertex3f(-500,-1,500);
    glColor3f(0.3f, 0.5f, 0.8f);
    glVertex3f(-500,200,500); glVertex3f(-500,200,-500);

    glColor3f(0.6f, 0.75f, 0.9f);
    glVertex3f(500,-1,-500); glVertex3f(500,-1,500);
    glColor3f(0.3f, 0.5f, 0.8f);
    glVertex3f(500,200,500); glVertex3f(500,200,-500);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void Renderer::drawGround(float size) {
    glDisable(GL_LIGHTING);

    // Tiled road/ground
    int tiles = 40;
    float tileSize = size * 2.f / tiles;

    glBegin(GL_QUADS);
    for (int x = 0; x < tiles; x++)
    for (int z = 0; z < tiles; z++) {
        float wx = -size + x * tileSize;
        float wz = -size + z * tileSize;
        bool road = (x % 5 == 2 || z % 5 == 2);
        if (road) glColor3f(0.25f, 0.25f, 0.27f);
        else      glColor3f(0.30f, 0.32f, 0.28f);
        glVertex3f(wx,          0, wz);
        glVertex3f(wx+tileSize, 0, wz);
        glVertex3f(wx+tileSize, 0, wz+tileSize);
        glVertex3f(wx,          0, wz+tileSize);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

void Renderer::drawCube(Vec3 pos, Vec3 hs, Vec3 color, float alpha) {
    glColor4f(color.x, color.y, color.z, alpha);
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    float x=hs.x, y=hs.y, z=hs.z;

    glBegin(GL_QUADS);
    // Front
    glNormal3f(0,0,1);
    glVertex3f(-x,-y, z); glVertex3f( x,-y, z);
    glVertex3f( x, y, z); glVertex3f(-x, y, z);
    // Back
    glNormal3f(0,0,-1);
    glVertex3f( x,-y,-z); glVertex3f(-x,-y,-z);
    glVertex3f(-x, y,-z); glVertex3f( x, y,-z);
    // Left
    glNormal3f(-1,0,0);
    glVertex3f(-x,-y,-z); glVertex3f(-x,-y, z);
    glVertex3f(-x, y, z); glVertex3f(-x, y,-z);
    // Right
    glNormal3f(1,0,0);
    glVertex3f( x,-y, z); glVertex3f( x,-y,-z);
    glVertex3f( x, y,-z); glVertex3f( x, y, z);
    // Top
    glNormal3f(0,1,0);
    glVertex3f(-x, y, z); glVertex3f( x, y, z);
    glVertex3f( x, y,-z); glVertex3f(-x, y,-z);
    // Bottom
    glNormal3f(0,-1,0);
    glVertex3f(-x,-y,-z); glVertex3f( x,-y,-z);
    glVertex3f( x,-y, z); glVertex3f(-x,-y, z);
    glEnd();

    glPopMatrix();
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

    // Main body
    drawCube({0,0,0}, hs, v.bodyColor);

    // Cab / top
    Vec3 cabSize = {hs.x * 0.7f, hs.y * 0.5f, hs.z * 0.5f};
    drawCube({0, hs.y + cabSize.y, 0}, cabSize, v.accentColor);

    // Wheels
    Vec3 wc = {0.15f, 0.15f, 0.15f}; // dark grey
    float wy = -hs.y + 0.15f;
    drawCube({-hs.x-0.1f, wy,  hs.z*0.5f}, {0.15f,0.25f,0.35f}, wc);
    drawCube({ hs.x+0.1f, wy,  hs.z*0.5f}, {0.15f,0.25f,0.35f}, wc);
    drawCube({-hs.x-0.1f, wy, -hs.z*0.5f}, {0.15f,0.25f,0.35f}, wc);
    drawCube({ hs.x+0.1f, wy, -hs.z*0.5f}, {0.15f,0.25f,0.35f}, wc);

    // Bulldozer blade
    if (v.type == VehicleType::BULLDOZER) {
        drawCube({0, 0, hs.z + 0.3f}, {hs.x * 1.1f, hs.y * 0.8f, 0.2f},
                 {0.7f, 0.5f, 0.1f});
    }

    glPopMatrix();
}

void Renderer::drawPlayer(const Player& p, const Camera& cam) {
    // Don't draw player body if camera is very close (first person feel)
    Vec3 diff = p.position - cam.position;
    if (diff.length() < 3.f) return;

    glPushMatrix();
    glTranslatef(p.position.x, p.position.y, p.position.z);
    glRotatef(-p.yaw * 180.f / PI, 0, 1, 0);

    // Body
    drawCube({0, 0.5f, 0},  {0.3f, 0.5f, 0.2f}, {0.2f, 0.3f, 0.7f});
    // Head
    drawCube({0, 1.2f, 0},  {0.2f, 0.2f, 0.2f}, {0.85f, 0.7f, 0.6f});
    // Legs
    drawCube({-0.15f,-0.3f,0}, {0.12f,0.3f,0.12f}, {0.15f,0.15f,0.5f});
    drawCube({ 0.15f,-0.3f,0}, {0.12f,0.3f,0.12f}, {0.15f,0.15f,0.5f});

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

    // Draw larger debris as small cubes
    for (const auto& p : particles) {
        if (p.size < 0.2f) continue;
        float a = p.life / p.maxLife;
        drawCube(p.position, Vec3(p.size, p.size, p.size)*0.5f, p.color, a);
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

        // Draw expanding sphere as stacked circles
        int segs = 16;
        for (int ring = 0; ring < 4; ring++) {
            float ry = sinf((ring / 3.f - 0.5f) * PI) * r;
            float rr = cosf((ring / 3.f - 0.5f) * PI) * r;
            glColor4f(1.f, 0.4f + ring*0.1f, 0.0f, alpha * (1.f - ring*0.2f));
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < segs; i++) {
                float a = i * 2.f * PI / segs;
                glVertex3f(e.origin.x + cosf(a)*rr, e.origin.y + ry,
                           e.origin.z + sinf(a)*rr);
            }
            glEnd();
        }
    }

    glEnable(GL_LIGHTING);
}

void Renderer::drawExplosiveMarkers(const Player& p) {
    glDisable(GL_LIGHTING);
    for (const auto& e : p.placedExplosives) {
        if (e.detonated) continue;
        // Blinking red marker
        float blink = sinf((float)sf::Clock().getElapsedTime().asSeconds() * 8.f);
        float r = 0.7f + blink * 0.3f;
        drawCube(e.position, {0.2f, 0.2f, 0.2f}, {r, 0.1f, 0.1f});

        // Radius indicator ring
        glColor4f(1.f, 0.2f, 0.2f, 0.3f);
        int segs = 24;
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segs; i++) {
            float a = i * 2.f * PI / segs;
            glVertex3f(e.position.x + cosf(a)*e.radius,
                       0.1f,
                       e.position.z + sinf(a)*e.radius);
        }
        glEnd();
    }
    glEnable(GL_LIGHTING);
}
