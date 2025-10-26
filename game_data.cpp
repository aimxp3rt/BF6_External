#include "game_data.h"
#include <iostream>
#include <algorithm>

namespace BF6 {

GameData::GameData()
    : m_game_context(0)
    , m_entity_list(0)
    , m_local_player_addr(0)
    , m_view_matrix_addr(0)
    , m_max_distance(500.0f)
    , m_update_rate_ms(10)
    , m_last_update(0)
    , m_initialized(false)
{
    Initialize();
}

GameData::~GameData() {
}

bool GameData::Initialize() {


    if (!FindBaseAddresses()) {
        std::cerr << "[GameData] Failed to find base addresses" << std::endl;
        return false;
    }

    m_initialized = true;
    std::cout << "[GameData] Game data manager initialized" << std::endl;
    return true;
}

bool GameData::FindBaseAddresses() {
    // TODO: Pattern scan or use static offsets to find:
    // - Game context pointer
    // - Entity list pointer
    // - Local player pointer
    // - View matrix address

    mem::CR3();

    base_address = mem::base_address();
    if (base_address == 0) {
        return false;
    }

    // Placeholder offsets (need to be found via reverse engineering)
    m_game_context = base_address + Offsets::GAME_CONTEXT;
    m_entity_list = base_address + Offsets::ENTITY_LIST;
    m_local_player_addr = base_address + Offsets::LOCAL_PLAYER;
    m_view_matrix_addr = base_address + Offsets::VIEW_MATRIX;

    std::cout << "[GameData] Base addresses found : 0x" << std::hex << base_address << std::endl;
    std::cout << "  Game Context: 0x" << std::hex << m_game_context << std::endl;
    std::cout << "  Entity List: 0x" << m_entity_list << std::endl;
    std::cout << "  Local Player: 0x" << m_local_player_addr << std::endl;
    std::cout << "  View Matrix: 0x" << m_view_matrix_addr << std::endl;

    return true;
}

bool GameData::Update() {
    if (!m_initialized) {
        return false;
    }

    // Rate limiting
    DWORD current_time = GetTickCount();
    if (current_time - m_last_update < m_update_rate_ms) {
        return true;
    }
    m_last_update = current_time;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Update game state
    if (!UpdateViewMatrix()) {
        return false;
    }

    if (!UpdateLocalPlayer()) {
        return false;
    }

    if (!UpdatePlayers()) {
        return false;
    }

    // Calculate screen positions for all entities
    CalculateScreenPositions();

    return true;
}

bool GameData::UpdateViewMatrix() {
    // Read view matrix from game memory
    return mem::ReadMemory(m_view_matrix_addr, &m_view_matrix, sizeof(ViewMatrix));
}

bool GameData::UpdateLocalPlayer() {
    // Read local player pointer
    uint64_t local_player_ptr = mem::Read<uint64_t>(m_local_player_addr);
    if (local_player_ptr == 0) {
        return false;
    }

    // Read local player data
    m_local_player.address = local_player_ptr;
    m_local_player.is_local = true;
    
    return ReadPlayerEntity(local_player_ptr, m_local_player);
}

bool GameData::UpdatePlayers() {
    m_players.clear();

    // Read entity list pointer
    uint64_t entity_list_ptr = mem::Read<uint64_t>(m_entity_list);
    if (entity_list_ptr == 0) {
        return false;
    }

    // Read entity count
    uint32_t entity_count = mem::Read<uint32_t>(entity_list_ptr + 0x8);
    if (entity_count == 0 || entity_count > 128) {
        return false;
    }

    // Read entity array
    uint64_t entity_array = mem::Read<uint64_t>(entity_list_ptr);
    if (entity_array == 0) {
        return false;
    }

    // Iterate through entities
    for (uint32_t i = 0; i < entity_count; i++) {
        uint64_t entity_addr = mem::Read<uint64_t>(entity_array + (i * 0x8));
        if (entity_addr == 0 || entity_addr == m_local_player.address) {
            continue;
        }

        PlayerEntity player;
        if (ReadPlayerEntity(entity_addr, player)) {
            // Filter by distance
            if (player.distance <= m_max_distance && player.IsValid()) {
                m_players.push_back(player);
            }
        }
    }

    // Sort by distance (closest first)
    std::sort(m_players.begin(), m_players.end(), 
        [](const PlayerEntity& a, const PlayerEntity& b) {
            return a.distance < b.distance;
        });

    return true;
}

bool GameData::ReadPlayerEntity(uint64_t address, PlayerEntity& player) {
    player.address = address;

    // Read basic data
    player.health = mem::Read<float>(address + Offsets::Player::HEALTH);
    player.max_health = mem::Read<float>(address + Offsets::Player::MAX_HEALTH);
    player.team_id = mem::Read<uint32_t>(address + Offsets::Player::TEAM_ID);
    player.is_alive = mem::Read<bool>(address + Offsets::Player::IS_ALIVE);
    player.is_visible = mem::Read<bool>(address + Offsets::Player::IS_VISIBLE);

    // Read position
    mem::ReadMemory(address + Offsets::Player::POSITION, &player.position, sizeof(Vector3));

    // Read velocity
    mem::ReadMemory(address + Offsets::Player::VELOCITY, &player.velocity, sizeof(Vector3));

    // Read name
    uint64_t name_ptr = mem::Read<uint64_t>(address + Offsets::Player::NAME);
    if (name_ptr != 0) {
        mem::ReadMemory(name_ptr, player.name, sizeof(player.name) - 1);
    }

    // Calculate distance from local player
    if (m_local_player.address != 0) {
        player.distance = player.position.Distance(m_local_player.position);
    }

    return player.IsValid();
}

bool GameData::ReadBones(uint64_t bone_component, std::vector<Bone>& bones) {
    if (bone_component == 0) {
        return false;
    }

    // Read bone array pointer
    uint64_t bone_array = mem::Read<uint64_t>(bone_component + Offsets::Bones::BONE_ARRAY);
    if (bone_array == 0) {
        return false;
    }

    // Read bone count
    uint32_t bone_count = mem::Read<uint32_t>(bone_component + Offsets::Bones::BONE_COUNT);
    if (bone_count == 0 || bone_count > 256) {
        return false;
    }

    bones.resize(bone_count);

    // Read all bones
    for (uint32_t i = 0; i < bone_count; i++) {
        uint64_t bone_addr = bone_array + (i * sizeof(Vector3));
        mem::ReadMemory(bone_addr, &bones[i].position, sizeof(Vector3));
        bones[i].parent_index = -1; // TODO: Read parent index if available
    }

    return true;
}

void GameData::CalculateScreenPositions() {
    // Get screen dimensions (assume 1920x1080 for now)
    int screen_width = 1920;
    int screen_height = 1080;

    // Calculate screen positions for all players
    for (auto& player : m_players) {
        // Body position
        m_view_matrix.WorldToScreen(player.position, player.screen_pos, 
                                     screen_width, screen_height);

        // Head position (approximate)
        Vector3 head_pos = player.position;
        head_pos.z += 1.8f; // Average head height
        m_view_matrix.WorldToScreen(head_pos, player.head_screen_pos, 
                                     screen_width, screen_height);
    }
}

} // namespace BF6

