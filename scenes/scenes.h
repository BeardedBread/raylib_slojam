#ifndef SCENES_H
#define SCENES_H
#include "engine.h"

typedef struct StoreItem
{
    float cost;
    int8_t cap;
    int8_t remaining;
} StoreItem;

typedef struct UpgradeStoreInventory
{
    StoreItem firerate_upgrade;
    StoreItem projspeed_upgrade;
    StoreItem damage_upgrade;
    StoreItem health_upgrade;
    StoreItem full_heal;
    StoreItem escape;
} UpgradeStoreInventory;

typedef struct LevelSceneData {
    RenderTexture2D game_viewport;
    Camera2D cam;
    Rectangle game_rec;
    Vector2 game_field_size;
    RenderTexture2D shop_viewport;
    Rectangle shop_rec;
    UpgradeStoreInventory store;
}LevelSceneData_t;

typedef struct LevelScene {
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_level_scene(LevelScene_t* scene);
void free_level_scene(LevelScene_t* scene);
#endif // SCENES_H
