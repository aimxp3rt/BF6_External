#pragma once
#include "imgui/imgui.h"
#include "game_data.h"
#include <memory>

namespace BF6 {

// Radar configuration
struct RadarConfig {
    // Enable/disable
    bool enabled;
    
    // Position and size
    float pos_x;
    float pos_y;
    float size;
    
    // Zoom and range
    float zoom;
    float max_range;
    
    // Appearance
    float background_alpha;
    float border_thickness;
    bool show_grid;
    int grid_lines;
    
    // Player dots
    float dot_size;
    bool show_names;
    bool show_distance;
    bool show_direction;
    
    // Colors
    ImVec4 background_color;
    ImVec4 border_color;
    ImVec4 grid_color;
    ImVec4 local_player_color;
    ImVec4 enemy_color;
    ImVec4 team_color;
    
    // Filters
    bool show_team;
    bool show_dead;
    
    RadarConfig();
};

// Radar renderer class
class RadarRenderer {
public:
    RadarRenderer(std::shared_ptr<GameData> game_data);
    ~RadarRenderer();

    // Render radar
    void Render();
    
    // Configuration
    RadarConfig& GetConfig() { return m_config; }
    void SetConfig(const RadarConfig& config) { m_config = config; }

private:
    std::shared_ptr<GameData> m_game_data;
    RadarConfig m_config;
    
    // Rendering functions
    void RenderBackground();
    void RenderGrid();
    void RenderPlayers();
    void RenderPlayer(const PlayerEntity& player, bool is_local);
    void RenderLocalPlayer();
    
    // Helper functions
    Vector2 WorldToRadar(const Vector3& world_pos, const Vector3& local_pos, float local_yaw);
    ImVec2 RadarToScreen(const Vector2& radar_pos);
    bool IsInRadarRange(const Vector3& world_pos, const Vector3& local_pos);
    ImVec4 GetPlayerColor(const PlayerEntity& player, bool is_local_team);
    
    // Drawing helpers
    void DrawCircle(const ImVec2& center, float radius, const ImVec4& color, bool filled = true);
    void DrawLine(const ImVec2& from, const ImVec2& to, const ImVec4& color, float thickness = 1.0f);
    void DrawText(const ImVec2& pos, const char* text, const ImVec4& color);
};

} // namespace BF6

