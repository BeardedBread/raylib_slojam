#include "engine.h"
#include "level_ent.h"

#include "scenes.h"
#include "raymath.h"

struct SpawnData
{
    float countdown_timer;
    uint16_t spawned;
};

void split_spawn_logic_func(Entity_t* self, SpawnerData* data, void* scene)
{
    data->spawned++;
}

void despawn_logic_func(Entity_t* self, SpawnerData* data, void* scene)
{
    data->spawned--;
}

void spawn_logic_func(Entity_t* self, SpawnerData* spwn_data, void* scene)
{
    LevelScene_t* lvl_scene = (LevelScene_t*)scene;

    if (spwn_data->spawned >= 15) return;

    spwn_data->countdown_timer -= lvl_scene->scene.delta_time;
    if (spwn_data->countdown_timer <= 0.0f)
    {
        float enemy_sz = 32;
        Entity_t* p_ent = create_enemy(&lvl_scene->scene.ent_manager, enemy_sz);
        CSpawn_t* p_spwn = add_component(p_ent, CSPAWNED_T);
        p_spwn->spawner = self;
        spwn_data->countdown_timer = 2.0f;
        spwn_data->spawned++;

        CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
        // Pick a side
        uint8_t side = GetRandomValue(0, 3);
        float speed = GetRandomValue(100, 300);
        float angle = 90.0f * side;
        switch (side)
        {
            case 0:
                p_ent->position = (Vector2){
                    -80,
                    GetRandomValue(0, lvl_scene->data.game_field_size.y - enemy_sz)
                };
            break;
            case 1:
                p_ent->position = (Vector2){
                    GetRandomValue(0, lvl_scene->data.game_field_size.x - enemy_sz),
                    -80
                };
            break;
            case 2:
                p_ent->position = (Vector2){
                    lvl_scene->data.game_field_size.x + 80,
                    GetRandomValue(0, lvl_scene->data.game_field_size.y - enemy_sz)
                };
            break;
            default:
                p_ent->position = (Vector2){
                    GetRandomValue(0, lvl_scene->data.game_field_size.x - enemy_sz),
                    lvl_scene->data.game_field_size.y + 80
                };
            break;
        }

        angle += GetRandomValue(-30, 30);
        angle = angle * PI / 180;
        p_ct->velocity = (Vector2){
            speed * cosf(angle),
            speed * sinf(angle),
        };

    }

}

Entity_t* create_spawner(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, NO_ENT_TAG);
    if (p_ent == NULL) return NULL;

    CSpawner_t* p_spawner = add_component(p_ent, CSPAWNER_T);
    p_spawner->spawner_main_logic = &spawn_logic_func;
    p_spawner->spawnee_despawn_logic = &despawn_logic_func;
    p_spawner->child_spawn_logic = &split_spawn_logic_func;
    return p_ent;
}

//void remove_spawner(Entity_t** ent)
//{
//    CAIFunction_t* p_ai = get_component(*ent, CAIFUNC_T);
//    if (p_ai != NULL)
//    {
//        if (p_ai->data != NULL)
//        {
//            free(p_ai->data);
//        }
//    }
//    remove_entity((*ent)->manager, (*ent)->m_id);
//    *ent = NULL;
//}
