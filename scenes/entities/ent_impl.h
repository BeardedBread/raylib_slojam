#ifndef _ENT_IMPL_H
#define _ENT_IMPL_H
#include <stdbool.h>
#include "assets.h"

typedef enum EntityTag {
    NO_ENT_TAG = 0,
    PLAYER_ENT_TAG,
} EntityTag_t;

Entity_t* create_player(EntityManager_t* ent_manager);

#endif // _ENT_IMPL_H
