#pragma once
#include "driver.h"
#include "bf6_structs.h"
#include <vector>
#include <memory>
#include <mutex>

namespace BF6 {

// Main game data manager - handles all entity reading and caching
class GameData {
public:
    GameData();
    ~GameData();

    // Update game state (call this every frame)
    bool Update();

    // Getters
    const std::vector<PlayerEntity>& GetPlayers() const { return m_players; }
    const PlayerEntity& GetLocalPlayer() const { return m_local_player; }
    const ViewMatrix& GetViewMatrix() const { return m_view_matrix; }
    
    bool IsValid() const { return m_initialized; }
    int GetPlayerCount() const { return static_cast<int>(m_players.size()); }
    
    // Configuration
    void SetMaxDistance(float distance) { m_max_distance = distance; }
    void SetUpdateRate(int ms) { m_update_rate_ms = ms; }

private:

    
    // Game state
    std::vector<PlayerEntity> m_players;
    PlayerEntity m_local_player;
    ViewMatrix m_view_matrix;
    
    // Base addresses
    uint64_t m_game_context;
    uint64_t m_entity_list;
    uint64_t m_local_player_addr;
    uint64_t m_view_matrix_addr;
    
    // Configuration
    float m_max_distance;
    int m_update_rate_ms;
    DWORD m_last_update;
    
    // State
    bool m_initialized;
    std::mutex m_mutex;
    
    // Helper functions
    bool Initialize();
    bool FindBaseAddresses();
    bool UpdateViewMatrix();
    bool UpdateLocalPlayer();
    bool UpdatePlayers();
    bool ReadPlayerEntity(uint64_t address, PlayerEntity& player);
    bool ReadBones(uint64_t bone_component, std::vector<Bone>& bones);
    void CalculateScreenPositions();
};

} // namespace BF6

