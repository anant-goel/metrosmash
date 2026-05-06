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

    // ── Graphics settings panel (toggle: G key) ───────────────────────────
    struct GfxPanel {
        bool  visible         = false;
        bool  shadersEnabled  = true;
        bool  shadowsEnabled  = true;
        bool  ssaoEnabled     = true;
        bool  bloomEnabled    = true;
        bool  vignetteOn      = true;
        bool  grainOn         = true;
        bool  chromaticOn     = true;
        bool  rainEnabled     = false;
        float timeOfDay       = 0.6f;
        float fogDensity      = 0.006f;
        float exposure        = 1.2f;
        float bloomStrength   = 0.25f;
    } gfxPanel;

    float gameTime = 0.f;
    float dt       = 0.f;

private:
    unsigned int screenW = 1280, screenH = 720;

    void setupGL();
    void setMatrices(const Camera& cam);
    void setModelMatrix(Vec3 pos, float yawAngle = 0.f, Vec3 scl = {1,1,1});
    void pushIdentityModel();

    // ── Advanced pipeline passes ──────────────────────────────────────────
    void renderAdvanced(const World& world, const Camera& camera);
    void renderLegacy  (const World& world, const Camera& camera);
    void renderShadowPass(const World& world, const Camera& cam);
    void drawBuildingShadow(const Building& b);

    // ── Advanced draw helpers (PBR uniforms) ──────────────────────────────
    void drawGroundAdvanced();
    void drawBuildingAdvanced(const Building& b);
    void drawVehicleAdvanced(const Vehicle& v);
    void drawPlayerAdvanced(const Player& p, const Camera& cam);

    // ── Legacy draw helpers (unchanged GL fixed-function) ─────────────────
    void drawGround(float size);
    void drawBuilding(const Building& b);
    void drawVehicle(const Vehicle& v);
    void drawPlayer(const Player& p, const Camera& cam);
    void drawSkyboxLegacy();

    // ── Shared primitives ─────────────────────────────────────────────────
    void drawCube(Vec3 pos, Vec3 halfSize, Vec3 color, float alpha = 1.f);
    void drawCubeInstanced(Vec3 pos, Vec3 halfSize, Vec3 color,
                           float roughness, float metallic,
                           float emissive = 0.f,
                           bool  isGlass  = false,
                           bool  isWindow = false);

    // ── FX (unchanged) ─────────────────────────────────────────────────────
    void drawParticles      (const std::vector<ParticleEffect>&  particles);
    void drawExplosionFX    (const std::vector<ExplosionForce>&   explosions);
    void drawExplosiveMarkers(const Player& p);
    void drawShockwaves     (const std::vector<ShockwaveAnim>&    waves);
    void drawDebrisChunks   (const std::vector<DebrisAnim>&       chunks);
};
