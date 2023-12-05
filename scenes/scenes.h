#ifndef SCENES_H
#define SCENES_H
#include "engine.h"

typedef struct LevelSceneData {
    RenderTexture2D game_viewport;
    Rectangle game_rec;
    Vector2 game_field_size;
}LevelSceneData_t;

typedef struct LevelScene {
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_level_scene(LevelScene_t* scene);
void free_level_scene(LevelScene_t* scene);
#endif // SCENES_H
