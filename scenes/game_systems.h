#ifndef _GAME_SYSTEMS_H
#define _GAME_SYSTEMS_H
#include "scenes.h"
void player_movement_input_system(Scene_t* scene, float delta_time);
void global_external_forces_system(Scene_t* scene, float delta_time);
void movement_update_system(Scene_t* scene, float delta_time);
void player_dir_reset_system(Scene_t* scene, float delta_time);
#endif // _GAME_SYSTEMS_H
