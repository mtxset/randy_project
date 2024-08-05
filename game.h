/* date = July 31st 2024 11:26 am */

#ifndef GAME_H
#define GAME_H


#define PI 3.14159265359f
#define TAU (2.0f * PI32)

#define vec2 Vector2
#define vec3 Vector3
#define vec4 Vector4
#define m4 Matrix4

const vec3 v3_z = {0, 0, 1};

#include "memory.h"
#include "color.h"
#include "easing.h"

typedef union Rect2 {
  struct {
    vec2 start;
    vec2 end;
  };
} Rect2;

enum Entity_type {
  Entity_type_null,
  Entity_type_player,
  Entity_type_rock,
  Entity_type_tree,
  
  Entity_type_pickaxe,
  
  Entity_type_item_start,
  Entity_type_item_rock,
  Entity_type_item_wood,
  Entity_type_item_end,
  
  Entity_type_building_luberjack_hut,
  Entity_type_building_luberjack_hut_2x,
  Entity_type_building_luberjack_hut_4x
};

enum Sound_effect {
  Sound_effect_pickaxe_hit
};

typedef struct Particle {
  vec2 pos, velocity;
  vec4 color;
  
  f32 life_left;
} Particle;

typedef struct Entity {
  vec2 pos;
  vec2 velocity;
  
  f32 angle;
  f32 scale;
  
  struct {
    vec2 start_pos;
    vec2 current_offset; // for animation
    vec2 size;
  } atlas;
  
  enum Entity_type type;
  
  f32 health;
  f32 max_health;
  
  bool is_valid;
  
  bool shaking;
  f32 shake_timer;
  vec2 shake_offset;
  vec2 shake_before_pos;
  
  vec4 color;
  
  f32 initial_scale;
  f32 start_y;
  f32 end_y;
  f32 landing_progress;
  bool animate_landing;
} Entity;

typedef struct Camera {
  vec2 pos;
  f32 angle;
  f32 zoom;
} Camera;

typedef struct Cell {
  u32 x, y;
} Cell;

typedef struct Game_state {
  Memory_arena arena;
  Memory_arena grid_arena;
  f32 time;
  f32 periodic_time;
  
  Gfx_Font *font;
  Gfx_Image *atlas;
  
  Entity *entity_list;
  u32 entity_count;
  
  Entity *player_entity;
  
  Cell   *filled_cell_list;
  u32    filled_count;
  
  Entity **grid_list;
  
  Camera camera;
  Camera ui_camera;
  
  struct {
    Entity *entity;
    Entity *hit_entity;
    
    f32 hit_speed;
    
    bool swinging;
    
    bool left;
    
    bool hit;
    
    bool first_hit;
    
    f32 step;
  } pickaxe;
  
  struct {
    Entity *entity;
    bool in_range;
    bool pressed;
  } on_entity;
  
  struct {
    bool show;
    
    vec2 pos;
    bool overlaps;
    
    bool pressed;
  } grid;
  
  struct {
    bool show;
    enum Entity_type selected_type;
    
  } ui_building;
  
  bool show_debug_lines;
  
  u32      particle_count;
  Particle *particle_list;
  
  bool is_initialized;
  
  //u64 _padding;
} Game_state;

#endif //GAME_H
