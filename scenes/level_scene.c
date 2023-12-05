#include "scenes.h"
#include "assets_tag.h"

#include <stdio.h>

static void level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    return;
}

static void level_scene_render_func(Scene_t* scene)
{
    static char buffer[32];
    float frame_time = GetFrameTime();
    snprintf(buffer, 32, "Ready to go: %.5f",frame_time);

    BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText(buffer, 64, 64, 24, RED);
    EndDrawing();
}

void init_level_scene(LevelScene_t* scene)
{
    init_scene(&scene->scene, &level_scene_render_func, &level_do_action);
    init_entity_tag_map(&scene->scene.ent_manager, PLAYER_ENT_TAG, 4);
}

void free_level_scene(LevelScene_t* scene)
{
    free_scene(&scene->scene);
}
