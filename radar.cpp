#include "radar.h"
#include <cmath>
#include "../Project1/imgui/imgui.h"

namespace BF6 {

RadarConfig::RadarConfig()
    : enabled(true)
    , pos_x(50.0f)
    , pos_y(50.0f)
    , size(250.0f)
    , zoom(1.0f)
    , max_range(200.0f)
    , background_alpha(0.7f)
    , border_thickness(2.0f)
    , show_grid(true)
    , grid_lines(4)
    , dot_size(6.0f)
    , show_names(false)
    , show_distance(true)
    , show_direction(true)
    , background_color(ImVec4(0.0f, 0.0f, 0.0f, 0.7f))
    , border_color(ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
    , grid_color(ImVec4(0.5f, 0.5f, 0.5f, 0.3f))
    , local_player_color(ImVec4(0.0f, 1.0f, 0.0f, 1.0f))
    , enemy_color(ImVec4(1.0f, 0.0f, 0.0f, 1.0f))
    , team_color(ImVec4(0.0f, 0.5f, 1.0f, 1.0f))
    , show_team(true)
    , show_dead(false)
{
}

RadarRenderer::RadarRenderer(std::shared_ptr<GameData> game_data)
    : m_game_data(game_data)
{
}

RadarRenderer::~RadarRenderer() {
}

void RadarRenderer::Render() {
    if (!m_config.enabled || !m_game_data || !m_game_data->IsValid()) {
        return;
    }

    RenderBackground();
    
    if (m_config.show_grid) {
        RenderGrid();
    }
    
    RenderPlayers();
    RenderLocalPlayer();
}

void RadarRenderer::RenderBackground() {
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    
    ImVec2 top_left(m_config.pos_x, m_config.pos_y);
    ImVec2 bottom_right(m_config.pos_x + m_config.size, m_config.pos_y + m_config.size);
    
    // Draw background
    ImU32 bg_color = ImGui::ColorConvertFloat4ToU32(m_config.background_color);
    draw_list->AddRectFilled(top_left, bottom_right, bg_color);
    
    // Draw border
    ImU32 border_color = ImGui::ColorConvertFloat4ToU32(m_config.border_color);
    draw_list->AddRect(top_left, bottom_right, border_color, 0.0f, 0, m_config.border_thickness);
}

void RadarRenderer::RenderGrid() {
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImU32 grid_color = ImGui::ColorConvertFloat4ToU32(m_config.grid_color);
    
    float step = m_config.size / m_config.grid_lines;
    
    // Vertical lines
    for (int i = 1; i < m_config.grid_lines; i++) {
        float x = m_config.pos_x + (i * step);
        ImVec2 from(x, m_config.pos_y);
        ImVec2 to(x, m_config.pos_y + m_config.size);
        draw_list->AddLine(from, to, grid_color, 1.0f);
    }
    
    // Horizontal lines
    for (int i = 1; i < m_config.grid_lines; i++) {
        float y = m_config.pos_y + (i * step);
        ImVec2 from(m_config.pos_x, y);
        ImVec2 to(m_config.pos_x + m_config.size, y);
        draw_list->AddLine(from, to, grid_color, 1.0f);
    }
    
    // Center crosshair
    float center_x = m_config.pos_x + m_config.size / 2;
    float center_y = m_config.pos_y + m_config.size / 2;
    float crosshair_size = 5.0f;
    
    draw_list->AddLine(ImVec2(center_x - crosshair_size, center_y), 
                       ImVec2(center_x + crosshair_size, center_y), grid_color, 2.0f);
    draw_list->AddLine(ImVec2(center_x, center_y - crosshair_size), 
                       ImVec2(center_x, center_y + crosshair_size), grid_color, 2.0f);
}

void RadarRenderer::RenderPlayers() {
    const auto& local_player = m_game_data->GetLocalPlayer();
    const auto& players = m_game_data->GetPlayers();
    
    for (const auto& player : players) {
        bool is_local_team = (player.team_id == local_player.team_id);
        
        // Filter by team
        if (!m_config.show_team && is_local_team) {
            continue;
        }
        
        // Filter by alive status
        if (!m_config.show_dead && !player.is_alive) {
            continue;
        }
        
        // Check if in range
        if (!IsInRadarRange(player.position, local_player.position)) {
            continue;
        }
        
        RenderPlayer(player, false);
    }
}

void RadarRenderer::RenderPlayer(const PlayerEntity& player, bool is_local) {
    const auto& local_player = m_game_data->GetLocalPlayer();
    
    // Convert world position to radar position
    Vector2 radar_pos = WorldToRadar(player.position, local_player.position, 0.0f);
    ImVec2 screen_pos = RadarToScreen(radar_pos);
    
    // Get color
    bool is_local_team = (player.team_id == local_player.team_id);
    ImVec4 color = GetPlayerColor(player, is_local_team);
    
    // Draw dot
    DrawCircle(screen_pos, m_config.dot_size, color, true);
    
    // Draw direction indicator if enabled
    if (m_config.show_direction) {
        // TODO: Calculate player facing direction and draw arrow
        // For now, just draw a small line
        float angle = 0.0f; // Player yaw angle
        float line_length = m_config.dot_size + 5.0f;
        ImVec2 dir_end(screen_pos.x + cosf(angle) * line_length,
                       screen_pos.y + sinf(angle) * line_length);
        DrawLine(screen_pos, dir_end, color, 2.0f);
    }
    
    // Draw name/distance if enabled
    if (m_config.show_names || m_config.show_distance) {
        char text[64];
        if (m_config.show_names && m_config.show_distance) {
            sprintf_s(text, "%s [%.0fm]", player.name, player.distance);
        } else if (m_config.show_names) {
            sprintf_s(text, "%s", player.name);
        } else {
            sprintf_s(text, "%.0fm", player.distance);
        }
        
        ImVec2 text_pos(screen_pos.x, screen_pos.y + m_config.dot_size + 2);
        DrawText(text_pos, text, color);
    }
}

void RadarRenderer::RenderLocalPlayer() {
    // Draw local player at center
    float center_x = m_config.pos_x + m_config.size / 2;
    float center_y = m_config.pos_y + m_config.size / 2;
    
    ImVec2 center(center_x, center_y);
    DrawCircle(center, m_config.dot_size, m_config.local_player_color, true);
    
    // Draw direction indicator (pointing up = forward)
    float line_length = m_config.dot_size + 8.0f;
    ImVec2 dir_end(center_x, center_y - line_length);
    DrawLine(center, dir_end, m_config.local_player_color, 3.0f);
}

Vector2 RadarRenderer::WorldToRadar(const Vector3& world_pos, const Vector3& local_pos, float local_yaw) {
    // Calculate relative position
    float dx = world_pos.x - local_pos.x;
    float dy = world_pos.y - local_pos.y;
    
    // Rotate based on local player yaw (so forward is always up)
    float cos_yaw = cosf(-local_yaw);
    float sin_yaw = sinf(-local_yaw);
    
    float rotated_x = dx * cos_yaw - dy * sin_yaw;
    float rotated_y = dx * sin_yaw + dy * cos_yaw;
    
    // Scale to radar size
    float scale = (m_config.size / 2) / (m_config.max_range * m_config.zoom);
    
    Vector2 radar_pos;
    radar_pos.x = rotated_x * scale;
    radar_pos.y = -rotated_y * scale; // Invert Y for screen coordinates
    
    return radar_pos;
}

ImVec2 RadarRenderer::RadarToScreen(const Vector2& radar_pos) {
    float center_x = m_config.pos_x + m_config.size / 2;
    float center_y = m_config.pos_y + m_config.size / 2;
    
    return ImVec2(center_x + radar_pos.x, center_y + radar_pos.y);
}

bool RadarRenderer::IsInRadarRange(const Vector3& world_pos, const Vector3& local_pos) {
    float dx = world_pos.x - local_pos.x;
    float dy = world_pos.y - local_pos.y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    return distance <= m_config.max_range;
}

ImVec4 RadarRenderer::GetPlayerColor(const PlayerEntity& player, bool is_local_team) {
    if (is_local_team) {
        return m_config.team_color;
    } else {
        return m_config.enemy_color;
    }
}

void RadarRenderer::DrawCircle(const ImVec2& center, float radius, const ImVec4& color, bool filled) {
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
    
    if (filled) {
        draw_list->AddCircleFilled(center, radius, col, 16);
    } else {
        draw_list->AddCircle(center, radius, col, 16, 2.0f);
    }
}

void RadarRenderer::DrawLine(const ImVec2& from, const ImVec2& to, const ImVec4& color, float thickness) {
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
    draw_list->AddLine(from, to, col, thickness);
}

void RadarRenderer::DrawText(const ImVec2& pos, const char* text, const ImVec4& color) {
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
    
    // Center text
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImVec2 centered_pos(pos.x - text_size.x / 2, pos.y);
    
    // Draw with outline
    draw_list->AddText(ImVec2(centered_pos.x + 1, centered_pos.y + 1), 
                       ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text);
    draw_list->AddText(centered_pos, col, text);
}

} // namespace BF6

