#include "esp.h"
#include "../Project1/imgui/imgui.h"
#include <algorithm>

namespace BF6 {

ESPConfig::ESPConfig()
    : box_enabled(true)
    , box_filled(false)
    , box_thickness(2.0f)
    , skeleton_enabled(true)
    , skeleton_thickness(2.0f)
    , health_bar_enabled(true)
    , health_bar_width(4)
    , health_bar_height(50)
    , name_enabled(true)
    , distance_enabled(true)
    , snapline_enabled(false)
    , snapline_position(2) // Bottom
    , enemy_color(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)) // Red
    , team_color(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)) // Green
    , visible_color(ImVec4(1.0f, 1.0f, 0.0f, 1.0f)) // Yellow
    , health_color_high(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)) // Green
    , health_color_low(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)) // Red
    , max_distance(500.0f)
    , show_team(false)
    , show_dead(false)
    , visible_only(false)
{
}

ESPRenderer::ESPRenderer(std::shared_ptr<GameData> game_data)
    : m_game_data(game_data)
{
}

ESPRenderer::~ESPRenderer() {
}

void ESPRenderer::Render() {
    if (!m_game_data || !m_game_data->IsValid()) {
        return;
    }

    const auto& local_player = m_game_data->GetLocalPlayer();
    const auto& players = m_game_data->GetPlayers();

    // Render all players
    for (const auto& player : players) {
        bool is_local_team = (player.team_id == local_player.team_id);
        
        if (ShouldRenderPlayer(player, is_local_team)) {
            RenderPlayer(player, is_local_team);
        }
    }
}

void ESPRenderer::RenderPlayer(const PlayerEntity& player, bool is_local_team) {
    ImVec4 color = GetPlayerColor(player, is_local_team);

    // Render snapline first (behind everything)
    if (m_config.snapline_enabled) {
        RenderSnapline(player, color);
    }

    // Render box
    if (m_config.box_enabled) {
        RenderBox(player, color);
    }

    // Render skeleton
    if (m_config.skeleton_enabled) {
        RenderSkeleton(player, color);
    }

    // Render health bar
    if (m_config.health_bar_enabled) {
        RenderHealthBar(player);
    }

    // Render name and distance
    if (m_config.name_enabled || m_config.distance_enabled) {
        RenderName(player, color);
    }
}

void ESPRenderer::RenderBox(const PlayerEntity& player, const ImVec4& color) {
    if (!player.screen_pos.IsValid() || !player.head_screen_pos.IsValid()) {
        return;
    }

    // Calculate box dimensions
    float height = player.screen_pos.y - player.head_screen_pos.y;
    float width = height * 0.4f; // Box width is 40% of height

    ImVec2 top_left(player.head_screen_pos.x - width / 2, player.head_screen_pos.y);
    ImVec2 bottom_right(player.head_screen_pos.x + width / 2, player.screen_pos.y);

    DrawBox(top_left, bottom_right, color, m_config.box_thickness, m_config.box_filled);
}

void ESPRenderer::RenderSkeleton(const PlayerEntity& player, const ImVec4& color) {
    // TODO: Implement skeleton rendering
    // Requires reading bone positions and converting to screen space
    // For now, just draw a simple line from head to feet
    
    if (!player.screen_pos.IsValid() || !player.head_screen_pos.IsValid()) {
        return;
    }

    ImVec2 head(player.head_screen_pos.x, player.head_screen_pos.y);
    ImVec2 feet(player.screen_pos.x, player.screen_pos.y);
    
    DrawLine(head, feet, color, m_config.skeleton_thickness);
}

void ESPRenderer::RenderHealthBar(const PlayerEntity& player) {
    if (!player.screen_pos.IsValid() || !player.head_screen_pos.IsValid()) {
        return;
    }

    float health_percent = player.GetHealthPercent();
    ImVec4 health_color = GetHealthColor(health_percent);

    // Calculate bar position (left side of box)
    float height = player.screen_pos.y - player.head_screen_pos.y;
    float width = height * 0.4f;
    
    float bar_x = player.head_screen_pos.x - width / 2 - m_config.health_bar_width - 2;
    float bar_y = player.head_screen_pos.y;
    float bar_height = height * (health_percent / 100.0f);

    // Draw background (black)
    ImVec2 bg_top(bar_x, bar_y);
    ImVec2 bg_bottom(bar_x + m_config.health_bar_width, bar_y + height);
    DrawBox(bg_top, bg_bottom, ImVec4(0, 0, 0, 0.5f), 1.0f, true);

    // Draw health bar
    ImVec2 health_top(bar_x, bar_y + height - bar_height);
    ImVec2 health_bottom(bar_x + m_config.health_bar_width, bar_y + height);
    DrawBox(health_top, health_bottom, health_color, 1.0f, true);
}

void ESPRenderer::RenderName(const PlayerEntity& player, const ImVec4& color) {
    if (!player.head_screen_pos.IsValid()) {
        return;
    }

    char text[128];
    if (m_config.name_enabled && m_config.distance_enabled) {
        sprintf_s(text, "%s [%.0fm]", player.name, player.distance);
    } else if (m_config.name_enabled) {
        sprintf_s(text, "%s", player.name);
    } else if (m_config.distance_enabled) {
        sprintf_s(text, "[%.0fm]", player.distance);
    } else {
        return;
    }

    // Draw text above head
    ImVec2 text_pos(player.head_screen_pos.x, player.head_screen_pos.y - 15);
    DrawText(text_pos, text, color);
}

void ESPRenderer::RenderSnapline(const PlayerEntity& player, const ImVec4& color) {
    if (!player.screen_pos.IsValid()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screen_center(io.DisplaySize.x / 2, io.DisplaySize.y / 2);
    
    ImVec2 start;
    switch (m_config.snapline_position) {
        case 0: // Top
            start = ImVec2(screen_center.x, 0);
            break;
        case 1: // Center
            start = screen_center;
            break;
        case 2: // Bottom
        default:
            start = ImVec2(screen_center.x, io.DisplaySize.y);
            break;
    }

    ImVec2 end(player.screen_pos.x, player.screen_pos.y);
    DrawLine(start, end, color, 1.0f);
}

ImVec4 ESPRenderer::GetPlayerColor(const PlayerEntity& player, bool is_local_team) {
    // Visible check overrides team color
    if (m_config.visible_only && player.is_visible) {
        return m_config.visible_color;
    }

    // Team vs enemy color
    if (is_local_team) {
        return m_config.team_color;
    } else {
        return m_config.enemy_color;
    }
}

ImVec4 ESPRenderer::GetHealthColor(float health_percent) {
    // Interpolate between low and high health colors
    float t = health_percent / 100.0f;
    
    ImVec4 color;
    color.x = m_config.health_color_low.x * (1 - t) + m_config.health_color_high.x * t;
    color.y = m_config.health_color_low.y * (1 - t) + m_config.health_color_high.y * t;
    color.z = m_config.health_color_low.z * (1 - t) + m_config.health_color_high.z * t;
    color.w = 1.0f;
    
    return color;
}

bool ESPRenderer::ShouldRenderPlayer(const PlayerEntity& player, bool is_local_team) {
    // Filter by team
    if (!m_config.show_team && is_local_team) {
        return false;
    }

    // Filter by alive status
    if (!m_config.show_dead && !player.is_alive) {
        return false;
    }

    // Filter by visibility
    if (m_config.visible_only && !player.is_visible) {
        return false;
    }

    // Filter by distance
    if (player.distance > m_config.max_distance) {
        return false;
    }

    // Check if on screen
    if (!player.screen_pos.IsValid()) {
        return false;
    }

    return true;
}

// Drawing helper implementations
void ESPRenderer::DrawBox(const ImVec2& top_left, const ImVec2& bottom_right, 
                          const ImVec4& color, float thickness, bool filled) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

    if (filled) {
        draw_list->AddRectFilled(top_left, bottom_right, col);
    } else {
        draw_list->AddRect(top_left, bottom_right, col, 0.0f, 0, thickness);
    }
}

void ESPRenderer::DrawLine(const ImVec2& from, const ImVec2& to, 
                           const ImVec4& color, float thickness) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
    draw_list->AddLine(from, to, col, thickness);
}

void ESPRenderer::DrawText(const ImVec2& pos, const char* text, const ImVec4& color) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
    
    // Center text
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImVec2 centered_pos(pos.x - text_size.x / 2, pos.y);
    
    // Draw text with outline for readability
    draw_list->AddText(ImVec2(centered_pos.x + 1, centered_pos.y + 1), 
                       ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text);
    draw_list->AddText(centered_pos, col, text);
}

void ESPRenderer::DrawCircle(const ImVec2& center, float radius, 
                             const ImVec4& color, float thickness) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
    draw_list->AddCircle(center, radius, col, 32, thickness);
}

} // namespace BF6

