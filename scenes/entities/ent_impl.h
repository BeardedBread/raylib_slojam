#ifndef _ENT_IMPL_H
#define _ENT_IMPL_H
#include <stdbool.h>
#include "assets.h"

typedef enum EntityTag {
    NO_ENT_TAG = 0,
    PLAYER_ENT_TAG,
    BULLET_ENT_TAG,
    ENEMY_ENT_TAG,
    COLLECT_ENT_TAG,
    UTIL_ENT_TAG,
} EntityTag_t;

bool init_player_creation(Assets_t* assets);
bool init_bullet_creation(Assets_t* assets);
bool init_enemies_creation(Assets_t* assets);
bool init_collectible_creation(Assets_t* assets);

Entity_t* create_player(EntityManager_t* ent_manager);
Entity_t* create_bullet(EntityManager_t* ent_manager);
Entity_t* create_enemy(EntityManager_t* ent_manager, float size, int32_t value);
Entity_t* create_final_enemy(EntityManager_t* ent_manager, float size, Vector2 pos);
Entity_t* create_start_target(EntityManager_t* ent_manager, float size, Vector2 pos);
Entity_t* create_collectible(EntityManager_t* ent_manager, float size, int32_t value);
Entity_t* create_end_item(EntityManager_t* ent_manager, float size);

void homing_target_func(Entity_t* self, void* scene);
#endif // _ENT_IMPL_H
