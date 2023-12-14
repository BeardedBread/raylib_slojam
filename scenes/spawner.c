#include "engine.h"
#include "level_ent.h"

#include "scenes.h"
#include "raymath.h"

struct SpawnData
{
    float countdown_timer;
    uint16_t spawned;
};

void despawn_logic_func(Entity_t* self, void* data, void* scene)
{
    struct SpawnData* spwn_data = (struct SpawnData*)data;

    spwn_data->spawned--;
}

void spawn_logic_func(Entity_t* self, void* data, void* scene)
{
    LevelScene_t* lvl_scene = (LevelScene_t*)scene;
    struct SpawnData* spwn_data = (struct SpawnData*)data;

    if (spwn_data->spawned == 15) return;

    spwn_data->countdown_timer -= lvl_scene->scene.delta_time;
    if (spwn_data->countdown_timer <= 0.0f)
    {
        Entity_t* p_ent = create_enemy(&lvl_scene->scene.ent_manager);
        CSpawn_t* p_spwn = add_component(p_ent, CSPAWNED_T);
        p_spwn->spawner = self;
        p_ent->position = (Vector2){
            GetRandomValue(0, lvl_scene->data.game_field_size.x),
            GetRandomValue(0, lvl_scene->data.game_field_size.y)
        };
        spwn_data->countdown_timer = 1.0f;
        spwn_data->spawned++;
    }

}

Entity_t* create_spawner(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, NO_ENT_TAG);
    if (p_ent == NULL) return NULL;

    CAIFunction_t* p_ai = add_component(p_ent, CAIFUNC_T);
    p_ai->data = calloc(1, sizeof(struct SpawnData));
    p_ai->func[0] = &spawn_logic_func;
    p_ai->func[1] = &despawn_logic_func;
    return p_ent;
}

void remove_spawner(Entity_t** ent)
{
    CAIFunction_t* p_ai = get_component(*ent, CAIFUNC_T);
    if (p_ai != NULL)
    {
        if (p_ai->data != NULL)
        {
            free(p_ai->data);
        }
    }
    remove_entity((*ent)->manager, (*ent)->m_id);
    *ent = NULL;
}

