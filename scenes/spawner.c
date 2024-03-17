#include "engine.h"
#include "level_ent.h"
#include "assets_tag.h"
#include "constants.h"
#include "scenes.h"
#include "ai_functions.h"
#include "raymath.h"

struct SpawnVariantData {
    uint16_t max_spawn;
    uint8_t prob;
};

struct RankSpawnData {
    uint32_t rank_up_value;
    float spawn_time_range[2];
    uint8_t size_chances[4];
    uint8_t speed_chances[3];
    uint8_t max_spawns;
    float drop_modifier;
    struct SpawnVariantData variant_data[2];
};

static const uint32_t SIZE_RANGES[5] = {12,18,32,56,80};
static const uint32_t SPEED_RANGES[4] = {100,200,250,320};
static const struct RankSpawnData RANK_DATA[MAX_RANK] = {
    {50, {5,1}, {10,100,100,100}, {100,100,100}, 7, 1.0f, {{0,0},{0,0}}},
    {150, {4,1}, {0,100,100,100}, {70,100,100}, 12, 1.1f, {{0,0},{0,0}}},
    {250, {3,2}, {0,85,100,100}, {15,100,100}, 13, 1.2f, {{3,10},{0,0}}},
    {400, {3,1}, {0,25,100,100}, {0,85,100}, 16, 1.4f, {{5,10}, {0,5}}},
    {600, {3,1}, {0,15,80,100}, {10,65,100}, 18, 1.6f, {{7,15},{0,10}}},
    {800, {2,2}, {0,0,90,100}, {0,80,100}, 20, 1.8f, {{10,18},{0,15}}},
    {1000, {2,1}, {0,10,85,100}, {10,70,100}, 22, 2.1f, {{12,20},{0,20}}},
    {2000, {2,1}, {0,0,70,100}, {10,50,100}, 24, 2.4f, {{14,25},{0,25}}},
};

static inline void make_enemy_maws(Entity_t* p_ent)
{
    CAIFunction_t* p_ai = add_component(p_ent, CAIFUNC_T);
    p_ai->target_idx = MAX_ENTITIES;
    p_ai->target_tag = PLAYER_ENT_TAG;
    p_ai->func = &homing_target_func;
    p_ai->accl = 800;

    CLifeTimer_t* p_life = get_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 1;
    p_life->max_life = 1;

    CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
    p_ct->shape_factor = 3;
    p_ct->velocity_cap = 500;
}

static inline void make_enemy_attract(Entity_t* p_ent)
{
    CAttract_t* p_attract = add_component(p_ent, CATTRACTOR_T);
    p_attract->attract_idx = 1;
    p_attract->attract_factor = 0.3f * p_ent->size / 32;
    p_attract->range_factor = 15.0 * p_ent->size / 32;

    CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
    p_cspr->colour = GRAY;
}

void split_spawn_logic_func(Entity_t* new_ent, SpawnerData* data, void* scene)
{
    data->spawned++;

    if (
        data->custom_counter[0] < RANK_DATA[data->rank].variant_data[0].max_spawn
        && GetRandomValue(0, 100) < RANK_DATA[data->rank].variant_data[0].prob
    )
    {
        make_enemy_attract(new_ent);
        data->custom_counter[0]++;
    }
    else if (GetRandomValue(0, 100) < RANK_DATA[data->rank].variant_data[1].prob) // Flat percent
    {
        make_enemy_maws(new_ent);
    }
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

    if (get_component(self, CATTRACTOR_T) != NULL)
    {
        data->custom_counter[0]--;
    }
}

void spawn_logic_func(Entity_t* self, SpawnerData* spwn_data, void* scene)
{
    LevelScene_t* lvl_scene = (LevelScene_t*)scene;
    if (spwn_data->rank < MAX_RANK - 1)
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
            play_sfx(lvl_scene->scene.engine, RANKUP_SFX, false);
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

        float value = 1.0; // always have 1
        value += enemy_sz / 3 + speed / 200.0;
        value *= RANK_DATA[spwn_data->rank].drop_modifier;
        Entity_t* p_ent = create_enemy(&lvl_scene->scene.ent_manager, enemy_sz, (int32_t)value);

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
                    GetRandomValue(enemy_sz, lvl_scene->data.game_field_size.y - enemy_sz)
                };
            break;
            case 1:
                p_ent->position = (Vector2){
                    GetRandomValue(enemy_sz, lvl_scene->data.game_field_size.x - enemy_sz),
                    -80
                };
            break;
            case 2:
                p_ent->position = (Vector2){
                    lvl_scene->data.game_field_size.x + 80,
                    GetRandomValue(enemy_sz, lvl_scene->data.game_field_size.y - enemy_sz)
                };
            break;
            default:
                p_ent->position = (Vector2){
                    GetRandomValue(enemy_sz, lvl_scene->data.game_field_size.x - enemy_sz),
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
    Entity_t* p_ent = add_entity(ent_manager, UTIL_ENT_TAG);
    if (p_ent == NULL) return NULL;

    CSpawner_t* p_spawner = add_component(p_ent, CSPAWNER_T);
    p_spawner->spawner_main_logic = &spawn_logic_func;
    p_spawner->spawnee_despawn_logic = &despawn_logic_func;
    p_spawner->child_spawn_logic = &split_spawn_logic_func;
    return p_ent;
}
