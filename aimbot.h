#pragma once

#include "game_data.h"
#include <memory>

namespace BF6 {

// Aimbot configuration
struct AimbotConfig {
    // Enable/disable
    bool enabled;
    int activation_key; // VK_XBUTTON2 or similar
    
    // Targeting
    enum TargetMode {
        CLOSEST_TO_CROSSHAIR,
        CLOSEST_DISTANCE,
        LOWEST_HEALTH
    };
    TargetMode target_mode;
    
    // Bone selection
    enum TargetBone {
        HEAD,
        NECK,
        CHEST,
        BODY_CENTER
    };
    TargetBone target_bone;
    
    // FOV settings
    bool fov_enabled;
    float fov_size; // Radius in pixels
    bool draw_fov;
    
    // Smoothing
    bool smooth_enabled;
    float smooth_value; // 1.0 = instant, higher = smoother
    
    // Filters
    bool visible_only;
    bool ignore_team;
    bool ignore_knocked;
    float max_distance;
    
    // Prediction
    bool velocity_prediction;
    float prediction_scale;
    
    AimbotConfig();
};

// Aimbot class - READ ONLY (no memory writes)
// Uses external mouse movement (KMBox or similar)
class Aimbot {
public:
    Aimbot(std::shared_ptr<GameData> game_data);
    ~Aimbot();

    // Update aimbot (call every frame)
    void Update();
    
    // Render FOV circle
    void RenderFOV();
    
    // Configuration
    AimbotConfig& GetConfig() { return m_config; }
    void SetConfig(const AimbotConfig& config) { m_config = config; }
    
    // Get current target
    const PlayerEntity* GetCurrentTarget() const { return m_current_target; }

private:
    std::shared_ptr<GameData> m_game_data;
    AimbotConfig m_config;
    const PlayerEntity* m_current_target;
    
    // Targeting functions
    const PlayerEntity* FindBestTarget();
    float GetTargetScore(const PlayerEntity& player, const Vector2& screen_center);
    bool IsValidTarget(const PlayerEntity& player);
    
    // Aiming functions
    Vector2 GetTargetAimPosition(const PlayerEntity& player);
    Vector2 CalculateAimAngles(const Vector2& current_pos, const Vector2& target_pos);
    void ApplySmoothing(Vector2& angles);
    
    // Prediction
    Vector3 PredictPosition(const PlayerEntity& player, float time);
    
    // Mouse movement (external device)
    void MoveMouse(const Vector2& delta);
    
    // Helper functions
    Vector2 GetScreenCenter();
    float GetDistanceToCenter(const Vector2& pos);
    bool IsKeyPressed(int vk_code);
};

} // namespace BF6

