#include "assets.h"
#include "assert.h"
#include "engine_conf.h"

#include <stdio.h>

uint8_t n_loaded[N_ASSETS_TYPE] = {0};

// Hard limit number of 
typedef struct TextureData
{
    Texture2D texture;
    char name[MAX_NAME_LEN];
}TextureData_t;
typedef struct SpriteData
{
    Sprite_t sprite;
    char name[MAX_NAME_LEN];
}SpriteData_t;
typedef struct FontData
{
    Font font;
    char name[MAX_NAME_LEN];
}FontData_t;
typedef struct SoundData
{
    Sound sound;
    char name[MAX_NAME_LEN];
}SoundData_t;
typedef struct LevelPackData
{
    LevelPack_t pack;
    char name[MAX_NAME_LEN];
}LevelPackData_t;
typedef struct EmitterConfData
{
    EmitterConfig_t conf;
    char name[MAX_NAME_LEN];
}EmitterConfData_t;

static TextureData_t textures[MAX_TEXTURES];
static FontData_t fonts[MAX_FONTS];
static SoundData_t sfx[MAX_SOUNDS];
static SpriteData_t sprites[MAX_SPRITES];
static EmitterConfData_t emitter_confs[MAX_EMITTER_CONF];

// Maybe need a circular buffer??
Texture2D* add_texture(Assets_t* assets, const char* name, const char* path)
{
    uint8_t tex_idx = n_loaded[AST_TEXTURE];
    assert(tex_idx < MAX_TEXTURES);
    Texture2D tex = LoadTexture(path);
    if (tex.width == 0 || tex.height == 0) return NULL;

    textures[tex_idx].texture = tex;
    strncpy(textures[tex_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_textures, textures[tex_idx].name, tex_idx);
    n_loaded[AST_TEXTURE]++;
    return &textures[tex_idx].texture;
}

Texture2D* add_texture_from_img(Assets_t* assets, const char* name, Image img)
{
    uint8_t tex_idx = n_loaded[AST_TEXTURE];
    assert(tex_idx < MAX_TEXTURES);
    Texture2D tex = LoadTextureFromImage(img);
    if (tex.width == 0 || tex.height == 0) return NULL;

    textures[tex_idx].texture = tex;
    strncpy(textures[tex_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_textures, textures[tex_idx].name, tex_idx);
    n_loaded[AST_TEXTURE]++;
    return &textures[tex_idx].texture;
}

Sprite_t* add_sprite(Assets_t* assets, const char* name, Texture2D* texture)
{
    uint8_t spr_idx = n_loaded[AST_SPRITE];
    assert(spr_idx < MAX_SPRITES);
    memset(sprites + spr_idx, 0, sizeof(SpriteData_t));
    sprites[spr_idx].sprite.texture = texture;
    strncpy(sprites[spr_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_sprites, sprites[spr_idx].name, spr_idx);
    n_loaded[AST_SPRITE]++;
    return &sprites[spr_idx].sprite;
}

Sound* add_sound(Assets_t* assets, const char* name, const char* path)
{
    uint8_t snd_idx = n_loaded[AST_SOUND];
    assert(snd_idx < MAX_SOUNDS);
    sfx[snd_idx].sound = LoadSound(path);
    strncpy(sfx[snd_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_sounds, sfx[snd_idx].name, snd_idx);
    n_loaded[AST_SOUND]++;
    return &sfx[snd_idx].sound;
}

Font* add_font(Assets_t* assets, const char* name, const char* path)
{
    uint8_t fnt_idx = n_loaded[AST_FONT];
    assert(fnt_idx < MAX_FONTS);
    fonts[fnt_idx].font = LoadFont(path);
    strncpy(fonts[fnt_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_fonts, fonts[fnt_idx].name, fnt_idx);
    n_loaded[AST_FONT]++;
    return &fonts[fnt_idx].font;
}

EmitterConfig_t* add_emitter_conf(Assets_t* assets, const char* name)
{
    uint8_t emitter_idx = n_loaded[AST_EMITTER_CONF];
    assert(emitter_idx < MAX_EMITTER_CONF);
    memset(emitter_confs + emitter_idx, 0, sizeof(EmitterConfData_t));
    strncpy(emitter_confs[emitter_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_emitter_confs, emitter_confs[emitter_idx].name, emitter_idx);
    n_loaded[AST_EMITTER_CONF]++;
    return &emitter_confs[emitter_idx].conf;
}

void init_assets(Assets_t* assets)
{
    sc_map_init_s64(&assets->m_fonts, MAX_FONTS, 0);
    sc_map_init_s64(&assets->m_sprites, MAX_SPRITES, 0);
    sc_map_init_s64(&assets->m_textures, MAX_TEXTURES, 0);
    sc_map_init_s64(&assets->m_sounds, MAX_SOUNDS, 0);
    sc_map_init_s64(&assets->m_emitter_confs, MAX_EMITTER_CONF, 0);
}

void free_all_assets(Assets_t* assets)
{
    for (uint8_t i = 0; i < n_loaded[AST_TEXTURE]; ++i)
    {
        UnloadTexture(textures[i].texture);
    }
    for (uint8_t i = 0; i < n_loaded[AST_SOUND]; ++i)
    {
        UnloadSound(sfx[i].sound);
    }
    for (uint8_t i = 0; i < n_loaded[AST_FONT]; ++i)
    {
        UnloadFont(fonts[i].font);
    }

    sc_map_clear_s64(&assets->m_textures);
    sc_map_clear_s64(&assets->m_fonts);
    sc_map_clear_s64(&assets->m_sounds);
    sc_map_clear_s64(&assets->m_sprites);
    sc_map_clear_s64(&assets->m_emitter_confs);
    memset(n_loaded, 0, sizeof(n_loaded));
}

void term_assets(Assets_t* assets)
{
    free_all_assets(assets);
    sc_map_term_s64(&assets->m_textures);
    sc_map_term_s64(&assets->m_fonts);
    sc_map_term_s64(&assets->m_sounds);
    sc_map_term_s64(&assets->m_sprites);
    sc_map_term_s64(&assets->m_emitter_confs);
}

Texture2D* get_texture(Assets_t* assets, const char* name)
{
    uint8_t tex_idx = sc_map_get_s64(&assets->m_textures, name);
    if (sc_map_found(&assets->m_textures))
    {
        return &textures[tex_idx].texture;
    }
    return NULL;
}

Sprite_t* get_sprite(Assets_t* assets, const char* name)
{
    uint8_t spr_idx = sc_map_get_s64(&assets->m_sprites, name);
    if (sc_map_found(&assets->m_sprites))
    {
        return &sprites[spr_idx].sprite;
    }
    return NULL;
}

EmitterConfig_t* get_emitter_conf(Assets_t* assets, const char* name)
{
    uint8_t emitter_idx = sc_map_get_s64(&assets->m_emitter_confs, name);
    if (sc_map_found(&assets->m_emitter_confs))
    {
        return &emitter_confs[emitter_idx].conf;
    }
    return NULL;
}


Sound* get_sound(Assets_t* assets, const char* name)
{
    uint8_t snd_idx = sc_map_get_s64(&assets->m_sounds, name);
    if (sc_map_found(&assets->m_sounds))
    {
        return &sfx[snd_idx].sound;
    }
    return NULL;
}

Font* get_font(Assets_t* assets, const char* name)
{
    uint8_t fnt_idx = sc_map_get_s64(&assets->m_fonts, name);
    if (sc_map_found(&assets->m_fonts))
    {
        return &fonts[fnt_idx].font;
    }
    return NULL;
}

void draw_sprite(Sprite_t* spr, int frame_num, Vector2 pos, float rotation, bool flip_x, Color colour)
{
    if (frame_num >= spr->frame_count) frame_num = spr->frame_count - 1;
    if (frame_num < 0) frame_num = 0;

    Rectangle rec = {
        spr->origin.x + spr->frame_size.x * frame_num,
        spr->origin.y,
        spr->frame_size.x * (flip_x ? -1:1),
        spr->frame_size.y
    };
    //DrawTextureRec(*spr->texture, rec, pos, WHITE);
    Rectangle dest = {
        .x = pos.x,
        .y = pos.y,
        .width = spr->frame_size.x,
        .height = spr->frame_size.y
    };
    DrawTexturePro(
        *spr->texture,
        rec,
        dest,
        spr->anchor,
        rotation, colour
    );
}

void draw_sprite_scaled(Sprite_t* spr, int frame_num, Vector2 pos, float rotation, float scale, Color colour)
{
    Rectangle rec = {
        spr->origin.x + spr->frame_size.x * frame_num,
        spr->origin.y,
        spr->frame_size.x,
        spr->frame_size.y
    };
    Rectangle dest = {
        .x = pos.x,
        .y = pos.y,
        .width = spr->frame_size.x * scale,
        .height = spr->frame_size.y * scale
    };
    Vector2 anchor = spr->anchor;
    anchor.x *= scale;
    anchor.y *= scale;
    DrawTexturePro(
        *spr->texture,
        rec,
        dest,
        anchor,
        rotation, colour
    );
}
