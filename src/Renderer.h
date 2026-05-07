#pragma once
#include "GameMath.h"
#include "World.h"
#include "Weapons.h"
#include <string>

namespace sf { class RenderWindow; }
class Camera;

class Renderer {
public:
    void init(unsigned int width, unsigned int height);
    void render(const World& world, const Camera& camera,
                const sf::RenderWindow& win);
    void resize(unsigned int w, unsigned int h);

    struct GfxPanel {
        bool  visible        = false;
        bool  shadersEnabled = false;
        bool  shadowsEnabled = true;
        bool  ssaoEnabled    = true;
        bool  bloomEnabled   = true;
        bool  vignetteOn     = true;
        bool  grainOn        = true;
        bool  chromaticOn    = true;
        bool  rainEnabled    = false;
        float timeOfDay      = 0.0f;   // 0=midday, 0.5=sunset, 1=night
        float fogDensity     = 0.006f;
        float exposure       = 1.2f;
        float bloomStrength  = 0.25f;
    } gfxPanel;

    float gameTime = 0.f;
    float dt       = 0.f;

private:
    unsigned int screenW = 1280, screenH = 720;

    void setupGL();
    void updateSunLight(float tod);
    void setMatrices(const Camera& cam);

    // ── Render paths ──────────────────────────────────────────────────────
    void renderLegacy  (const World& world, const Camera& camera);
    void renderAdvanced(const World& world, const Camera& camera); // stub

    // ── Advanced stubs (keep linker happy) ────────────────────────────────
    void renderShadowPass(const World&, const Camera&);
    void drawBuildingShadow(const Building&);
    void drawGroundAdvanced();
    void drawBuildingAdvanced(const Building&);
    void drawVehicleAdvanced(const Vehicle&);
    void drawPlayerAdvanced(const Player&, const Camera&);
    void drawSkyboxLegacy();
    void drawParticles(const std::vector<ParticleEffect>&);
    void drawExplosionFX(const std::vector<ExplosionForce>&);
    void drawCubeInstanced(Vec3,Vec3,Vec3,float,float,float,bool,bool);

    // ── Active draw methods ───────────────────────────────────────────────
    void drawSky(float tod, float hr, float hg, float hb,
                 float zr, float zg, float zb, const Camera& cam);
    void drawGround(float tod);                       // enhanced, takes tod
    void drawBuilding(const Building& b, float tod);  // enhanced
    void drawVehicle(const Vehicle& v);               // kept
    void drawPlayer(const Player& p, const Camera& cam); // kept

    void drawVegetation(const std::vector<VegetationNode>& nodes, float time);
    void drawPedestrians(const std::vector<Pedestrian>& peds);

    void drawCube(Vec3 pos, Vec3 halfSize, Vec3 color, float alpha = 1.f);

    void drawExplosionFX(const std::vector<ExplosionForce>& explosions,
                         const std::vector<ParticleEffect>& particles);
    void drawExplosiveMarkers(const Player& p);
    void drawShockwaves(const std::vector<ShockwaveAnim>& waves);
    void drawDebrisChunks(const std::vector<DebrisAnim>& chunks);
    void drawActiveWeapons(const std::vector<ActiveWeapon>& active);
    void drawRain(Vec3 cam, float tod);
};
