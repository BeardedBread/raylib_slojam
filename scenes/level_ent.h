#ifndef _LEVEL_ENT_H
#define _LEVEL_ENT_H
#include "ent_impl.h"
Entity_t* create_spawner(EntityManager_t* ent_manager);
void set_spawn_level(SpawnerData* spwn_data, uint8_t lvl);
//void remove_spawner(Entity_t** ent);

#endif // _LEVEL_ENT_H
