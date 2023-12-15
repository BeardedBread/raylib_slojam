#ifndef _ENT_IMPL_H
#define _ENT_IMPL_H
#include <stdbool.h>
#include "assets.h"

typedef enum EntityTag {
    NO_ENT_TAG = 0,
    PLAYER_ENT_TAG,
    ENEMY_ENT_TAG,
} EntityTag_t;

Entity_t* create_player(EntityManager_t* ent_manager);
Entity_t* create_bullet(EntityManager_t* ent_manager);
Entity_t* create_enemy(EntityManager_t* ent_manager, float size);

#endif // _ENT_IMPL_H
