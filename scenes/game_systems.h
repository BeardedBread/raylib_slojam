#ifndef _GAME_SYSTEMS_H
#define _GAME_SYSTEMS_H
#include "scenes.h"
void weapon_cooldown_system(Scene_t* scene);
void player_movement_input_system(Scene_t* scene);
void global_external_forces_system(Scene_t* scene);
void movement_update_system(Scene_t* scene);
void hitbox_update_system(Scene_t* scene);
void player_dir_reset_system(Scene_t* scene);
#endif // _GAME_SYSTEMS_H
