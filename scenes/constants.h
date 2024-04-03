// Constants to be used in game
#ifndef __CONSTANTS_H
#define __CONSTANTS_H
#define GRAV_ACCEL 1500
#define JUMP_SPEED 70
#define MOVE_ACCEL 500
#define PLAYER_MAX_SPEED 300

#define PLAYER_WIDTH 30
#define PLAYER_HEIGHT 42
#define PLAYER_C_WIDTH 30
#define PLAYER_C_HEIGHT 26

#define ARENA_WIDTH 1200
#define ARENA_HEIGHT 1000

#define CRIT_HEALTH 10
#define MIN_BOOST_SPEED 500
#define HEALTH_INCREMENT 15
#define N_HEALTH_UPGRADES 3
#define MAXIMUM_HEALTH (30 + N_HEALTH_UPGRADES * HEALTH_INCREMENT)

#define N_WEAPONS 4
#define N_UPGRADES 8

#define SIZE_SPLIT_FACTOR 1.75f
#define ENEMY_MIN_SIZE 13
#define BOOST_COOLDOWN 3.0f

#define MAX_RANK 10
#endif // __CONSTANTS_H
