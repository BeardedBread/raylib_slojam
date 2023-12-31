#ifndef __ENGINE_H
#define __ENGINE_H
#include "actions.h"
#include "sc/array/sc_array.h"
#include "assets.h"

typedef struct Scene Scene_t;

typedef struct SFXList
{
    SFX_t sfx[N_SFX];
    uint32_t sfx_queue[N_SFX];
    uint32_t n_sfx;
    uint32_t played_sfx;
} SFXList_t;

typedef struct GameEngine {
    Scene_t **scenes;
    unsigned int max_scenes;
    unsigned int curr_scene;
    Assets_t assets;
    SFXList_t sfx_list;
    // Maintain own queue to handle key presses
    struct sc_queue_32 key_buffer;
} GameEngine_t;

typedef enum SceneType {
    MAIN_SCENE = 0,
    SUB_SCENE,
}SceneType_t;

typedef enum SceneState {
    SCENE_PLAYING = 0, // Action, System, Render Active
    SCENE_PAUSED, // System Inactive, Action + Render Active
    SCENE_SUSPENDED, // All Inactive
    SCENE_ENDED, // All Inactive + Reset to default
}SceneState_t;

typedef enum ActionResult {
    ACTION_PROPAGATE = 0,
    ACTION_CONSUMED,
} ActionResult;

typedef void(*render_func_t)(Scene_t*);
typedef void(*system_func_t)(Scene_t*);
typedef ActionResult(*action_func_t)(Scene_t*, ActionType_t, bool);
sc_array_def(system_func_t, systems);

#define ACTION_BIT 1
#define SYSTEM_BIT (1 << 1)
#define RENDER_BIT (1 << 2)

struct Scene {
    struct sc_map_64 action_map; // key -> actions
    struct sc_array_systems systems;
    render_func_t render_function;
    action_func_t action_function;
    EntityManager_t ent_manager;
    float delta_time;
    float time_scale;
    Vector2 mouse_pos; // Relative to the scene
    //SceneState_t state;
    uint8_t state_bits;
    SceneType_t type;
    ParticleSystem_t part_sys;

    Scene_t* parent_scene;
    Scene_t* subscene;
    Vector2 subscene_pos;

    GameEngine_t *engine;
};

void init_engine(GameEngine_t* engine);
void deinit_engine(GameEngine_t* engine);
void process_inputs(GameEngine_t* engine, Scene_t* scene);

void change_scene(GameEngine_t* engine, unsigned int idx);
bool load_sfx(GameEngine_t* engine, const char* snd_name, uint32_t tag_idx);
void set_sfx_pitch(GameEngine_t* engine, uint32_t tag_idx, float pitch);
void play_sfx(GameEngine_t* engine, unsigned int tag_idx, bool allow_overlay);
void stop_sfx(GameEngine_t* engine, unsigned int tag_idx);
void update_sfx_list(GameEngine_t* engine);

// Inline functions, for convenience
extern void update_scene(Scene_t* scene, float delta_time);
extern void render_scene(Scene_t* scene);
extern void do_action(Scene_t* scene, ActionType_t action, bool pressed);

//void init_scene(Scene_t* scene, SceneType_t scene_type, system_func_t render_func, action_func_t action_func);
void init_scene(Scene_t* scene, render_func_t render_func, action_func_t action_func);
void free_scene(Scene_t* scene);

#endif // __ENGINE_H
