#pragma once
#include "Math.h"
#include "World.h"
#include "Animation.h"
#include <SFML/Graphics.hpp>
#include <GL/gl.h>
#include <string>

class Renderer {
public:
    void init(unsigned int width, unsigned int height);
    void render(const World& world, const Camera& camera, const sf::RenderWindow& win);
    void resize(unsigned int w, unsigned int h);

private:
    unsigned int screenW = 1280, screenH = 720;

    void setupGL();
    void setMatrices(const Camera& cam);

    // Draw primitives
    void drawCube(Vec3 pos, Vec3 halfSize, Vec3 color, float alpha = 1.f);
    void drawCubeWire(Vec3 pos, Vec3 halfSize, Vec3 color);
    void drawGround(float size);
    void drawBuilding(const Building& b);
    void drawVehicle(const Vehicle& v);
    void drawPlayer(const Player& p, const Camera& cam);
    void drawParticles(const std::vector<ParticleEffect>& particles);
    void drawExplosionFX(const std::vector<ExplosionForce>& expl);
    void drawExplosiveMarkers(const Player& p);
    void drawShockwaves(const std::vector<ShockwaveAnim>& waves);
    void drawDebrisChunks(const std::vector<DebrisAnim>& chunks);
    void drawSkybox();

    void pushMatrix(const Mat4& m);
    Mat4 currentModelMatrix;
};
