#include "engine.h"
#include "level_ent.h"

#include "scenes.h"
#include "raymath.h"

struct RankSpawnData {
    uint32_t rank_up_value;
    float spawn_time_range[2];
    uint8_t size_chances[4];
    uint8_t speed_chances[3];
    uint8_t max_spawns;
};

static const uint32_t SIZE_RANGES[5] = {10,18,32,56,98};
static const uint32_t SPEED_RANGES[4] = {100,200,300,400};

#define MAX_RANK 8
static const struct RankSpawnData RANK_DATA[MAX_RANK] = {
    {50, {5,1}, {10,100,100,100}, {100,100,100}, 7},
    {125, {4,2}, {5,100,100,100}, {90,100,100}, 10},
    {200, {4,1}, {5,85,100,100}, {70,100,100}, 13},
    {300, {3,2}, {0,75,100,100}, {20,85,100}, 15},
    {400, {3,1}, {0,15,100,100}, {5,85,100}, 18},
    {550, {2,2}, {0,5,90,100}, {0,80,100}, 22},
    {750, {2,1}, {0,0,75,100}, {0,60,100}, 30},
    {1000, {1,2}, {0,0,40,100}, {0,30,100}, 35},
};

void split_spawn_logic_func(Entity_t* self, SpawnerData* data, void* scene)
{
    data->spawned++;
}

void despawn_logic_func(Entity_t* self, SpawnerData* data, void* scene)
{
    CTransform_t* p_ct = get_component(self, CTRANSFORM_T);
    data->rank_counter += 1 + 50 / self->size + Vector2Length(p_ct->velocity) / 1000;
    if (data->spawned > 0)
    {
        data->spawned--;
    }

    if (data->spawned == 0)
    {
        data->spawn_timer /= 2;
    }
}

void spawn_logic_func(Entity_t* self, SpawnerData* spwn_data, void* scene)
{
    LevelScene_t* lvl_scene = (LevelScene_t*)scene;
    if (spwn_data->rank < MAX_RANK)
    {
        spwn_data->rank_timer += lvl_scene->scene.delta_time;
        if (spwn_data->rank_timer > 1.0f)
        {
            spwn_data->rank_counter++; // +1 every sec
            spwn_data->rank_timer -= 1.0f;
        }
        if (spwn_data->rank_counter >= RANK_DATA[spwn_data->rank].rank_up_value)
        {
            spwn_data->rank_counter -= RANK_DATA[spwn_data->rank].rank_up_value;
            spwn_data->rank++;
            // Reset spawning mechanics
            //spwn_data->spawned = 0;
            //spwn_data->spawn_timer = RANK_DATA[spwn_data->rank].spawn_time_range[0]
            //    + RANK_DATA[spwn_data->rank].spawn_time_range[1] * (float)rand() / (float)RAND_MAX;
        }
    }

    if (spwn_data->spawned >= RANK_DATA[spwn_data->rank].max_spawns) return;

    spwn_data->spawn_timer -= lvl_scene->scene.delta_time;
    if (spwn_data->spawn_timer <= 0.0f)
    {
        int chance = GetRandomValue(0, 100);
        uint8_t idx = 0;
        for (idx = 0; idx < 3; idx++)
        {
            if (chance <= RANK_DATA[spwn_data->rank].size_chances[idx]) break;
        }
        float enemy_sz = GetRandomValue(SIZE_RANGES[idx], SIZE_RANGES[idx+1]);

        chance = GetRandomValue(0, 100);
        for (idx = 0; idx < 2; idx++)
        {
            if (chance <= RANK_DATA[spwn_data->rank].speed_chances[idx]) break;
        }
        float speed = SPEED_RANGES[idx]
            + (SPEED_RANGES[idx+1] - SPEED_RANGES[idx])
            * (float)rand() / (float)RAND_MAX;

        spwn_data->spawned++;
        spwn_data->spawn_timer += RANK_DATA[spwn_data->rank].spawn_time_range[0]
                + RANK_DATA[spwn_data->rank].spawn_time_range[1] * (float)rand() / (float)RAND_MAX;

        Entity_t* p_ent = create_enemy(&lvl_scene->scene.ent_manager, enemy_sz);
        CSpawn_t* p_spwn = add_component(p_ent, CSPAWNED_T);
        p_spwn->spawner = self;

        CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
        // Pick a side
        uint8_t side = GetRandomValue(0, 3);
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
