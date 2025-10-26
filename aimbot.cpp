#include "aimbot.h"
#include "../Project1/imgui/imgui.h"
#include <windows.h>
#include <algorithm>
#include <cmath>

namespace BF6 {

AimbotConfig::AimbotConfig()
    : enabled(false)
    , activation_key(VK_XBUTTON2) // Mouse button 5
    , target_mode(CLOSEST_TO_CROSSHAIR)
    , target_bone(HEAD)
    , fov_enabled(true)
    , fov_size(100.0f)
    , draw_fov(true)
    , smooth_enabled(true)
    , smooth_value(5.0f)
    , visible_only(true)
    , ignore_team(true)
    , ignore_knocked(true)
    , max_distance(300.0f)
    , velocity_prediction(false)
    , prediction_scale(1.0f)
{
}

Aimbot::Aimbot(std::shared_ptr<GameData> game_data)
    : m_game_data(game_data)
    , m_current_target(nullptr)
{
}

Aimbot::~Aimbot() {
}

void Aimbot::Update() {
    if (!m_config.enabled || !m_game_data || !m_game_data->IsValid()) {
        m_current_target = nullptr;
        return;
    }

    // Check if activation key is pressed
    if (!IsKeyPressed(m_config.activation_key)) {
        m_current_target = nullptr;
        return;
    }

    // Find best target
    m_current_target = FindBestTarget();
    if (!m_current_target) {
        return;
    }

    // Get target aim position
    Vector2 target_pos = GetTargetAimPosition(*m_current_target);
    if (!target_pos.IsValid()) {
        m_current_target = nullptr;
        return;
    }

    // Calculate aim angles
    Vector2 screen_center = GetScreenCenter();
    Vector2 delta = CalculateAimAngles(screen_center, target_pos);

    // Apply smoothing
    if (m_config.smooth_enabled) {
        ApplySmoothing(delta);
    }

    // Move mouse (external device)
    MoveMouse(delta);
}

void Aimbot::RenderFOV() {
    if (!m_config.draw_fov || !m_config.fov_enabled) {
        return;
    }

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    Vector2 center = GetScreenCenter();
    
    ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
    draw_list->AddCircle(ImVec2(center.x, center.y), m_config.fov_size, color, 64, 2.0f);
}

const PlayerEntity* Aimbot::FindBestTarget() {
    const auto& players = m_game_data->GetPlayers();
    if (players.empty()) {
        return nullptr;
    }

    const PlayerEntity* best_target = nullptr;
    float best_score = FLT_MAX;
    Vector2 screen_center = GetScreenCenter();

    for (const auto& player : players) {
        if (!IsValidTarget(player)) {
            continue;
        }

        float score = GetTargetScore(player, screen_center);
        if (score < best_score) {
            best_score = score;
            best_target = &player;
        }
    }

    return best_target;
}

float Aimbot::GetTargetScore(const PlayerEntity& player, const Vector2& screen_center) {
    Vector2 target_pos = GetTargetAimPosition(player);
    
    switch (m_config.target_mode) {
        case AimbotConfig::CLOSEST_TO_CROSSHAIR:
            return GetDistanceToCenter(target_pos);
            
        case AimbotConfig::CLOSEST_DISTANCE:
            return player.distance;
            
        case AimbotConfig::LOWEST_HEALTH:
            return player.health;
            
        default:
            return GetDistanceToCenter(target_pos);
    }
}

bool Aimbot::IsValidTarget(const PlayerEntity& player) {
    // Check if player is valid
    if (!player.IsValid()) {
        return false;
    }

    // Check team
    if (m_config.ignore_team) {
        const auto& local_player = m_game_data->GetLocalPlayer();
        if (player.team_id == local_player.team_id) {
            return false;
        }
    }

    // Check visibility
    if (m_config.visible_only && !player.is_visible) {
        return false;
    }

    // Check knocked status
    if (m_config.ignore_knocked && player.health <= 0) {
        return false;
    }

    // Check distance
    if (player.distance > m_config.max_distance) {
        return false;
    }

    // Check FOV
    if (m_config.fov_enabled) {
        Vector2 target_pos = GetTargetAimPosition(player);
        if (GetDistanceToCenter(target_pos) > m_config.fov_size) {
            return false;
        }
    }

    // Check if on screen
    if (!player.screen_pos.IsValid()) {
        return false;
    }

    return true;
}

Vector2 Aimbot::GetTargetAimPosition(const PlayerEntity& player) {
    // Select bone based on configuration
    switch (m_config.target_bone) {
        case AimbotConfig::HEAD:
            return player.head_screen_pos;
            
        case AimbotConfig::NECK:
            // Approximate neck position (slightly below head)
            return Vector2(player.head_screen_pos.x, 
                          player.head_screen_pos.y + 10);
            
        case AimbotConfig::CHEST:
            // Approximate chest position (middle of body)
            {
                float height = player.screen_pos.y - player.head_screen_pos.y;
                return Vector2(player.head_screen_pos.x, 
                              player.head_screen_pos.y + height * 0.4f);
            }
            
        case AimbotConfig::BODY_CENTER:
        default:
            return player.screen_pos;
    }
}

Vector2 Aimbot::CalculateAimAngles(const Vector2& current_pos, const Vector2& target_pos) {
    // Calculate delta (how much to move mouse)
    Vector2 delta;
    delta.x = target_pos.x - current_pos.x;
    delta.y = target_pos.y - current_pos.y;
    
    // Apply velocity prediction if enabled
    if (m_config.velocity_prediction && m_current_target) {
        // Predict future position based on velocity
        Vector3 predicted_pos = PredictPosition(*m_current_target, 0.1f); // 100ms prediction
        
        // Convert to screen space and adjust delta
        const auto& view_matrix = m_game_data->GetViewMatrix();
        Vector2 predicted_screen;
        if (view_matrix.WorldToScreen(predicted_pos, predicted_screen, 1920, 1080)) {
            delta.x += (predicted_screen.x - target_pos.x) * m_config.prediction_scale;
            delta.y += (predicted_screen.y - target_pos.y) * m_config.prediction_scale;
        }
    }
    
    return delta;
}

void Aimbot::ApplySmoothing(Vector2& angles) {
    // Apply smoothing factor
    angles.x /= m_config.smooth_value;
    angles.y /= m_config.smooth_value;
}

Vector3 Aimbot::PredictPosition(const PlayerEntity& player, float time) {
    // Simple linear prediction
    Vector3 predicted = player.position;
    predicted.x += player.velocity.x * time;
    predicted.y += player.velocity.y * time;
    predicted.z += player.velocity.z * time;
    return predicted;
}

void Aimbot::MoveMouse(const Vector2& delta) {
    // NOTE: This is a READ-ONLY cheat, so we don't actually write to game memory
    // Instead, we use external mouse movement via KMBox or similar hardware
    
    // For KMBox integration, you would use their SDK:
    // kmbox_move(delta.x, delta.y);
    
    // For testing without hardware, you can use Windows mouse_event:
    // mouse_event(MOUSEEVENTF_MOVE, (DWORD)delta.x, (DWORD)delta.y, 0, 0);
    
    // Placeholder: just log the movement
    // In production, replace with actual KMBox or Arduino mouse emulation
    static int frame_count = 0;
    if (++frame_count % 60 == 0) { // Log every 60 frames
        // std::cout << "[Aimbot] Move: " << delta.x << ", " << delta.y << std::endl;
    }
}

Vector2 Aimbot::GetScreenCenter() {
    ImGuiIO& io = ImGui::GetIO();
    return Vector2(io.DisplaySize.x / 2, io.DisplaySize.y / 2);
}

float Aimbot::GetDistanceToCenter(const Vector2& pos) {
    Vector2 center = GetScreenCenter();
    float dx = pos.x - center.x;
    float dy = pos.y - center.y;
    return sqrtf(dx * dx + dy * dy);
}

bool Aimbot::IsKeyPressed(int vk_code) {
    return (GetAsyncKeyState(vk_code) & 0x8000) != 0;
}

} // namespace BF6

