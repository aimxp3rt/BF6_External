#pragma once

#include "game_data.h"
#include "../Project1/imgui/imgui.h"
#include <memory>
#include <d3d11.h>

// ImGui forward declarations
struct ImDrawList;
struct ImVec2;
struct ImVec4;

namespace BF6 {

// ESP configuration
struct ESPConfig {
    // Box ESP
    bool box_enabled;
    bool box_filled;
    float box_thickness;
    
    // Skeleton ESP
    bool skeleton_enabled;
    float skeleton_thickness;
    
    // Health bar
    bool health_bar_enabled;
    int health_bar_width;
    int health_bar_height;
    
    // Name ESP
    bool name_enabled;
    bool distance_enabled;
    
    // Snaplines
    bool snapline_enabled;
    int snapline_position; // 0=top, 1=center, 2=bottom
    
    // Colors
    ImVec4 enemy_color;
    ImVec4 team_color;
    ImVec4 visible_color;
    ImVec4 health_color_high;
    ImVec4 health_color_low;
    
    // Filters
    float max_distance;
    bool show_team;
    bool show_dead;
    bool visible_only;
    
    ESPConfig();
};

// ESP renderer class
class ESPRenderer {
public:
    ESPRenderer(std::shared_ptr<GameData> game_data);
    ~ESPRenderer();

    // Render ESP
    void Render();
    
    // Configuration
    ESPConfig& GetConfig() { return m_config; }
    void SetConfig(const ESPConfig& config) { m_config = config; }

private:
    std::shared_ptr<GameData> m_game_data;
    ESPConfig m_config;
    
    // Rendering functions
    void RenderPlayer(const PlayerEntity& player, bool is_local_team);
    void RenderBox(const PlayerEntity& player, const ImVec4& color);
    void RenderSkeleton(const PlayerEntity& player, const ImVec4& color);
    void RenderHealthBar(const PlayerEntity& player);
    void RenderName(const PlayerEntity& player, const ImVec4& color);
    void RenderSnapline(const PlayerEntity& player, const ImVec4& color);
    
    // Helper functions
    ImVec4 GetPlayerColor(const PlayerEntity& player, bool is_local_team);
    ImVec4 GetHealthColor(float health_percent);
    bool ShouldRenderPlayer(const PlayerEntity& player, bool is_local_team);
    
    // Drawing helpers
    void DrawBox(const ImVec2& top_left, const ImVec2& bottom_right, 
                 const ImVec4& color, float thickness, bool filled = false);
    void DrawLine(const ImVec2& from, const ImVec2& to, 
                  const ImVec4& color, float thickness);
    void DrawText(const ImVec2& pos, const char* text, const ImVec4& color);
    void DrawCircle(const ImVec2& center, float radius, 
                    const ImVec4& color, float thickness);
};

} // namespace BF6

