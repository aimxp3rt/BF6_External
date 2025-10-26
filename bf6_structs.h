#pragma once

#include <windows.h>
#include <stdint.h>
#include <DirectXMath.h>

using namespace DirectX;

namespace BF6 {

// ============================================================================
// OFFSETS - These need to be updated per game version
// Based on research from UnknownCheats and community sources
// ============================================================================

namespace Offsets {
    constexpr uint64_t GAME_CONTEXT = 0x9867338; // To be determined
    constexpr uint64_t ENTITY_LIST = 0x14B2C8B10; // To be determined
    constexpr uint64_t LOCAL_PLAYER = 0x14B2C8C20; // To be determined
    constexpr uint64_t VIEW_MATRIX = 0x14B2C8D30; // To be determined
    
    // Player/Entity offsets
    namespace Player {
        constexpr uint64_t HEALTH = 0x140;
        constexpr uint64_t MAX_HEALTH = 0x148;
        constexpr uint64_t TEAM_ID = 0x13C;
        constexpr uint64_t POSITION = 0x2D0;
        constexpr uint64_t ROTATION = 0x2E0;
        constexpr uint64_t VELOCITY = 0x2F0;
        constexpr uint64_t NAME = 0x40;
        constexpr uint64_t BONE_COMPONENT = 0x588;
        constexpr uint64_t WEAPON = 0x5B0;
        constexpr uint64_t IS_ALIVE = 0x1B8;
        constexpr uint64_t IS_VISIBLE = 0x5A1;
    }
    
    // Bone offsets for skeleton ESP
    namespace Bones {
        constexpr uint64_t BONE_ARRAY = 0x20;
        constexpr uint64_t BONE_COUNT = 0x28;
        constexpr uint64_t BONE_TRANSFORM = 0x0;
    }
    
    // Weapon offsets
    namespace Weapon {
        constexpr uint64_t WEAPON_DATA = 0x10;
        constexpr uint64_t AMMO_COUNT = 0x238;
        constexpr uint64_t MAX_AMMO = 0x23C;
        constexpr uint64_t WEAPON_NAME = 0x48;
    }
    
    // Vehicle offsets
    namespace Vehicle {
        constexpr uint64_t VEHICLE_ENTITY = 0x0;
        constexpr uint64_t VEHICLE_HEALTH = 0x140;
        constexpr uint64_t VEHICLE_TYPE = 0x50;
    }
}

// ============================================================================
// STRUCTURES
// ============================================================================

// 3D Vector
struct Vector3 {
    float x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    
    float Distance(const Vector3& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }
    
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
};

// 2D Vector for screen coordinates
struct Vector2 {
    float x, y;
    
    Vector2() : x(0), y(0) {}
    Vector2(float _x, float _y) : x(_x), y(_y) {}
    
    bool IsValid() const {
        return x > 0 && y > 0 && x < 10000 && y < 10000;
    }
};

// View Matrix for world-to-screen conversion
struct ViewMatrix {
    float matrix[4][4];
    
    bool WorldToScreen(const Vector3& world, Vector2& screen, int width, int height) const {
        float w = matrix[3][0] * world.x + matrix[3][1] * world.y + 
                  matrix[3][2] * world.z + matrix[3][3];
        
        if (w < 0.01f)
            return false;
        
        float x = matrix[0][0] * world.x + matrix[0][1] * world.y + 
                  matrix[0][2] * world.z + matrix[0][3];
        float y = matrix[1][0] * world.x + matrix[1][1] * world.y + 
                  matrix[1][2] * world.z + matrix[1][3];
        
        screen.x = (width / 2.0f) + (width / 2.0f) * x / w;
        screen.y = (height / 2.0f) - (height / 2.0f) * y / w;
        
        return screen.IsValid();
    }
};

// Player entity structure
struct PlayerEntity {
    uint64_t address;
    uint32_t team_id;
    float health;
    float max_health;
    Vector3 position;
    Vector3 velocity;
    char name[64];
    bool is_alive;
    bool is_visible;
    bool is_local;
    float distance;
    Vector2 screen_pos;
    Vector2 head_screen_pos;
    
    PlayerEntity() : address(0), team_id(0), health(0), max_health(100),
                     is_alive(false), is_visible(false), is_local(false), distance(0) {
        memset(name, 0, sizeof(name));
    }
    
    bool IsValid() const {
        return address != 0 && is_alive && health > 0;
    }
    
    bool IsEnemy(uint32_t local_team) const {
        return team_id != local_team && team_id != 0;
    }
    
    float GetHealthPercent() const {
        if (max_health <= 0) return 0;
        return (health / max_health) * 100.0f;
    }
};

// Bone structure for skeleton ESP
struct Bone {
    Vector3 position;
    int parent_index;
};

// Bone indices (common skeleton structure)
enum BoneIndex {
    HEAD = 0,
    NECK = 1,
    SPINE_UPPER = 2,
    SPINE_MID = 3,
    SPINE_LOWER = 4,
    PELVIS = 5,
    LEFT_SHOULDER = 6,
    LEFT_ELBOW = 7,
    LEFT_HAND = 8,
    RIGHT_SHOULDER = 9,
    RIGHT_ELBOW = 10,
    RIGHT_HAND = 11,
    LEFT_HIP = 12,
    LEFT_KNEE = 13,
    LEFT_FOOT = 14,
    RIGHT_HIP = 15,
    RIGHT_KNEE = 16,
    RIGHT_FOOT = 17,
    MAX_BONES = 18
};

// Weapon information
struct WeaponInfo {
    char name[64];
    int ammo_current;
    int ammo_max;
    
    WeaponInfo() : ammo_current(0), ammo_max(0) {
        memset(name, 0, sizeof(name));
    }
};

} // namespace BF6

