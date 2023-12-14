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
    //CBBOX_T,
    //CTILECOORD_T,
    CPLAYERSTATE_T,
    CCONTAINER_T,
    CHITBOXES_T,
    CHURTBOX_T,
    CSPRITE_T,
    CLIFETIMER_T,
    CEMITTER_T,
    CWEAPON_T,
    CAIFUNC_T,
    CSPAWNED_T
} ComponentEnum_t;

typedef struct _CBBox_t {
    Vector2 size;
    Vector2 offset;
    Vector2 half_size;
    bool solid;
    bool fragile;
} CBBox_t;

typedef struct _CTransform_t {
    Vector2 prev_position;
    Vector2 prev_velocity;
    Vector2 velocity;
    Vector2 accel;
    bool active;
    bool wraparound;
    float shape_factor;
} CTransform_t;

// This is to store the occupying tiles
// Limits to store 4 tiles at a tile,
// Thus the size of the entity cannot be larger than the tile
typedef struct _CTileCoord_t {
    unsigned int tiles[8];
    unsigned int n_tiles;
} CTileCoord_t;

typedef struct _CPlayerState_t {
    Vector2 player_dir;
    Vector2 aim_dir;
    uint8_t boosting;
    uint8_t shoot;
    float boost_cooldown;
} CPlayerState_t;

typedef enum ContainerItem {
    CONTAINER_EMPTY,
} ContainerItem_t;

typedef struct _CContainer_t {
    ContainerItem_t item;
    uint8_t layers;
} CContainer_t;

typedef enum DamageType
{
    DMG_MELEE = 0,
    DMG_PROJ,
}DamageType;

typedef struct _CWeapon {
    float base_dmg;
    float proj_speed;
    float fire_rate;
    float cooldown_timer;
} CWeapon_t;

typedef struct _CHitBoxes_t {
    Vector2 offset;
    float size;
    uint8_t atk;
    bool one_hit;
    DamageType dmg_type;
} CHitBoxes_t;

typedef struct _CHurtbox_t {
    Vector2 offset;
    float size;
    uint8_t def;
    unsigned int damage_src;
} CHurtbox_t;

typedef struct _CLifeTimer_t {
    int16_t current_life;
    int16_t max_life;
    int16_t corruption;
    int16_t poison_value;
} CLifeTimer_t;

// This is so bad
typedef void (*ai_func_t)(Entity_t* self, void* data, void* scene_data);
typedef struct _CAIFunction_t {
    void* scene_data;
    void* data;
    ai_func_t func[4]; // AI function slots, 0 - main, the rest is custom
} CAIFunction_t;

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
    int elapsed;
    int speed;
    char* name;
} Sprite_t;

typedef unsigned int (*sprite_transition_func_t)(Entity_t *ent); // Transition requires knowledge of the entity
typedef struct _SpriteRenderInfo
{
    Sprite_t* sprite;
    Vector2 offset;
} SpriteRenderInfo_t;

typedef struct _CSprite_t {
    SpriteRenderInfo_t* sprites;
    sprite_transition_func_t transition_func;
    unsigned int current_idx;
    bool flip_x;
    bool flip_y;
    bool pause;
    int current_frame;
} CSprite_t;

typedef uint16_t EmitterHandle;
typedef struct _CEmitter_t
{
    EmitterHandle handle;
    Vector2 offset;
} CEmitter_t;

static inline void set_bbox(CBBox_t* p_bbox, unsigned int x, unsigned int y)
{
    p_bbox->size.x = x;
    p_bbox->size.y = y;
    p_bbox->half_size.x = (unsigned int)(x / 2);
    p_bbox->half_size.y = (unsigned int)(y / 2);
}

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
