#ifndef __ENTITY_H
#define __ENTITY_H
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "sc/map/sc_map.h"
#include "sc/queue/sc_queue.h"

#include "engine_conf.h"

typedef struct EntityManager EntityManager_t;
typedef struct Entity Entity_t;

typedef enum ComponentEnum {
    CTRANSFORM_T = 0,
    CPLAYERSTATE_T,
    CCONTAINER_T,
    CHITBOXES_T,
    CHURTBOX_T,
    CATTRACTOR_T,
    CMAGNET_T,
    CMONEY_T,
    CSPRITE_T,
    CLIFETIMER_T,
    CEMITTER_T,
    CWEAPON_T,
    CWEAPONSTORE_T,
    CAIFUNC_T,
    CSPAWNER_T,
    CSPAWNED_T,
    CHOMING_T,
    CWALLET_T,
} ComponentEnum_t;

typedef enum EdgeBehaviour {
    EDGE_NORMAL = 0,
    EDGE_BOUNCE,
    EDGE_WRAPAROUND
}EdgeBehaviour_t;

typedef struct _CTransform_t {
    Vector2 prev_position;
    Vector2 prev_velocity;
    Vector2 velocity;
    float velocity_cap;
    Vector2 accel;
    bool active;
    EdgeBehaviour_t edge_b;
    float shape_factor;
    uint8_t boundary_checks;
    int8_t x_crossed;
    int8_t y_crossed;
} CTransform_t;

typedef struct _CPlayerState_t {
    Vector2 player_dir;
    Vector2 aim_dir;
    uint8_t moving;
    uint8_t boosting;
    uint8_t shoot;
    float boost_cooldown;
    uint32_t collected;
} CPlayerState_t;

typedef enum ContainerItem {
    CONTAINER_EMPTY = 0,
    CONTAINER_BULLETS,
    CONTAINER_ENEMY,
    CONTAINER_BOMB,
    CONTAINER_GOAL,
    CONTAINER_ENDING,
} ContainerItem_t;

typedef struct _CContainer_t {
    ContainerItem_t item;
    uint8_t num;
} CContainer_t;

typedef struct _CAttract_t {
    unsigned long attract_idx;
    float attract_factor;
    float range_factor;
} CAttract_t;

typedef struct _CMagnet_t {
    unsigned long attract_idx;
    float l2_range;
    float accel;
} CMagnet_t;

typedef struct _CWallet_t {
    int32_t value;
} CWallet_t;

typedef struct _CMoney_t {
    int32_t value;
    unsigned int collect_sound;
} CMoney_t;

typedef enum DamageType
{
    DMG_MELEE = 0,
    DMG_PROJ,
}DamageType;

typedef enum DamageSource
{
    DMGSRC_NEUTRAL = 0,
    DMGSRC_PLAYER,
    DMGSRC_ENEMY,
}DamageSource;

typedef struct _CWeapon {
    float base_dmg;
    float proj_speed;
    float fire_rate;
    float cooldown_timer;
    float hold_timer;
    float spread_range;
    float bullet_lifetime;
    float bullet_kb;
    uint8_t n_bullets;
    uint8_t weapon_idx;
    uint8_t special_prop;
    const uint8_t* modifiers;
} CWeapon_t;

typedef struct _CWeaponStore {
    uint8_t n_weapons;
    CWeapon_t weapons[8];
    bool unlocked[8];
    uint8_t modifier[8];
} CWeaponStore_t;

typedef enum HitBoxType {
    HITBOX_CIRCLE = 0, // Circular hitbox
    HITBOX_RAY, //Infinite range ray
} HitBoxType_t;

typedef struct _CHitBoxes_t {
    Vector2 offset;
    Vector2 dir;
    float size;
    float knockback;
    uint8_t atk;
    bool one_hit;
    HitBoxType_t type;
    DamageType dmg_type;
    DamageSource src;
    unsigned int hit_sound;
} CHitBoxes_t;

// All hurtbox are circular
typedef struct _CHurtbox_t {
    Vector2 offset;
    float size;
    float invuln_timer;
    float invuln_time;
    DamageSource src;
    unsigned int damage_src;
    Vector2 attack_dir;
    uint8_t def;
    float kb_mult;
} CHurtbox_t;

typedef struct _CLifeTimer_t {
    int16_t current_life;
    int16_t max_life;
    float corruption;
    float poison;
    float poison_value;
} CLifeTimer_t;

// This is so bad
typedef void (*decision_func_t)(Entity_t* self, void* data, void* scene_data);
typedef struct _CAIFunction_t {
    void* data;
    decision_func_t func;
} CAIFunction_t;

typedef struct SpawnerData
{
    uint8_t rank;
    uint32_t rank_counter;
    float spawn_timer;
    float rank_timer;
    uint16_t spawned;
} SpawnerData;

typedef void (*spawner_func_t)(Entity_t* self, SpawnerData* data, void* scene_data);
typedef struct _CSpawner_t {
    SpawnerData data;
    spawner_func_t spawner_main_logic;
    spawner_func_t spawnee_despawn_logic;
    spawner_func_t child_spawn_logic;
} CSpawner_t;

typedef struct _CHoming_t {
    unsigned long target_idx;
    Vector2 target_pos;
    Vector2 target_vel;
} CHoming_t;

typedef struct _CSpawn_t {
    Entity_t* spawner;
} CSpawn_t;

// Credits to bedroomcoders.co.uk for this
typedef struct Sprite {
    Texture2D* texture;
    Vector2 frame_size;
    Vector2 origin;
    Vector2 anchor;
    int frame_count;
    int speed;
    char* name;
} Sprite_t;

typedef unsigned int (*sprite_transition_func_t)(Entity_t *ent); // Transition requires knowledge of the entity
typedef struct _CSprite_t {
    Sprite_t** sprites;
    sprite_transition_func_t transition_func;
    unsigned int current_idx;
    bool flip_x;
    bool flip_y;
    bool pause;
    int current_frame;
    float rotation; // Degree
    float rotation_speed; // Degree / s
    int elapsed;
    Vector2 offset;
} CSprite_t;

typedef uint16_t EmitterHandle;
typedef struct _CEmitter_t
{
    EmitterHandle handle;
    Vector2 offset;
} CEmitter_t;

struct Entity {
    Vector2 spawn_pos;
    unsigned long m_id;
    unsigned int m_tag;
    unsigned long components[N_COMPONENTS]; 
    EntityManager_t* manager;
    bool m_alive;
    bool m_active;
    Vector2 position;
    float size;
};

enum EntityUpdateEvent
{
    COMP_ADDTION,
    COMP_DELETION,
};

struct EntityUpdateEventInfo
{
    unsigned long e_id;
    ComponentEnum_t comp_type;
    unsigned long c_id;
    enum EntityUpdateEvent evt_type;
};

sc_queue_def(struct EntityUpdateEventInfo, ent_evt);

struct EntityManager {
    // All fields are Read-Only
    struct sc_map_64v entities; // ent id : entity
    struct sc_map_64v entities_map[N_TAGS]; // [{ent id: ent}]
    bool tag_map_inited[N_TAGS];
    struct sc_map_64v component_map[N_COMPONENTS]; // [{ent id: comp}, ...]
    struct sc_queue_uint to_add;
    struct sc_queue_uint to_remove;
    struct sc_queue_ent_evt to_update;
};

void init_entity_manager(EntityManager_t* p_manager);
void init_entity_tag_map(EntityManager_t* p_manager, unsigned int tag_number, unsigned int initial_size);
void update_entity_manager(EntityManager_t* p_manager);
void clear_entity_manager(EntityManager_t* p_manager);
void free_entity_manager(EntityManager_t* p_manager);

Entity_t* add_entity(EntityManager_t* p_manager, unsigned int tag);
void remove_entity(EntityManager_t* p_manager, unsigned long id);
Entity_t *get_entity(EntityManager_t* p_manager, unsigned long id);

void* add_component(Entity_t *entity, ComponentEnum_t comp_type);
void* get_component(Entity_t *entity, ComponentEnum_t comp_type);
void remove_component(Entity_t* entity, ComponentEnum_t comp_type);

#endif // __ENTITY_H
