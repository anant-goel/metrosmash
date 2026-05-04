#pragma once
// NO SFML includes here - forward declare only to avoid miniaudio.h cmath poisoning
#include "Math.h"
#include "World.h"
#include <string>

// Forward declare SFML types used in interface
namespace sf { class RenderWindow; }

class Camera;

class Renderer {
public:
    void init(unsigned int width, unsigned int height);
    void render(const World& world, const Camera& camera, const sf::RenderWindow& win);
    void resize(unsigned int w, unsigned int h);

private:
    unsigned int screenW = 1280, screenH = 720;
    void setupGL();
    void setMatrices(const Camera& cam);
    void drawCube(Vec3 pos, Vec3 halfSize, Vec3 color, float alpha = 1.f);
    void drawGround(float size);
    void drawBuilding(const Building& b);
    void drawVehicle(const Vehicle& v);
    void drawPlayer(const Player& p, const Camera& cam);
    void drawParticles(const std::vector<ParticleEffect>& particles);
    void drawExplosionFX(const std::vector<ExplosionForce>& expl);
    void drawExplosiveMarkers(const Player& p);
    void drawSkybox();
    void drawShockwaves(const std::vector<ShockwaveAnim>& waves);
    void drawDebrisChunks(const std::vector<DebrisAnim>& chunks);
};
