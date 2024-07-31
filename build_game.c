#define OOGABOOGA_LINK_EXTERNAL_INSTANCE 1
#include "oogabooga/oogabooga.c"

#include "game.h"

const vec4 no_over_color = {0,0,0,0};

inline
bool
rect_overlaps(vec4 rect, vec2 pos) {
  return (pos.x >= rect.x1 && pos.x < rect.x2 &&
          pos.y >= rect.y1 && pos.y < rect.y2);
}

inline
f32
random_f32(f32 min_value, f32 max_value) {
  return get_random_float32_in_range(min_value, max_value);
}

#define for_each_entity(game_state, entity) \
for (u32 entity_index = 0; entity_index < (game_state)->entity_count; entity_index++) \
for (Entity *entity = (game_state)->entity_list + entity_index; entity && (entity)->is_valid; entity = NULL)

inline
m4
identity() {
  m4 result = m4_scalar(1.0);
  return result;
}

static
vec4
calc_atlas_offset(vec2 current_offset_px, vec2 sprite_size_px, vec2 texture_size) {
  
  vec4 result = {};
  
  vec2 start_pos = { 
    current_offset_px.x / texture_size.x,  
    (texture_size.y - current_offset_px.y) / texture_size.y  // start top-left
  };
  
  vec2 sprite_size = {
    sprite_size_px.x / texture_size.x, 
    sprite_size_px.y / texture_size.y 
  };
  
  result.xy = start_pos;
  result.zw = v2_add(start_pos, sprite_size);
  
  return result;
}

inline
vec2
ndc_coords(vec2 coords) {
  
  vec2 result = {
    (2.0f * coords.x) / window.width - 1.0f,
    (2.0f * coords.y) / window.height - 1.0f,
  };
  
  return result;
}


static
void
render_line_no_ndc(vec2 point, vec2 end_point, vec4 color) {
  
  f32 line_width = 0.005f;
  
  vec2 ndc_point_a = point;
  vec2 ndc_point_b = end_point;
  
  vec2 p0 = ndc_point_a;
  vec2 p1 = ndc_point_b;
  
  Vector2 dir = v2(p1.x - p0.x, p1.y - p0.y);
  float length = sqrt(dir.x * dir.x + dir.y * dir.y);
  float r = atan2(-dir.y, dir.x);
  Matrix4 line_xform = m4_scalar(1);
  line_xform = m4_translate(line_xform, v3(ndc_point_a.x, ndc_point_a.y, 0));
  line_xform = m4_rotate_z(line_xform, r);
  line_xform = m4_translate(line_xform, v3(0, -line_width/2, 0));
  draw_rect_xform(line_xform, v2(length, line_width), color);
}

static
void
render_line(vec2 point, vec2 end_point, vec4 color) {
  
  f32 line_width = 0.005f;
  
  vec2 ndc_point_a = ndc_coords(point);
  vec2 ndc_point_b = ndc_coords(end_point);
  
  vec2 p0 = ndc_point_a;
  vec2 p1 = ndc_point_b;
  
  Vector2 dir = v2(p1.x - p0.x, p1.y - p0.y);
  float length = sqrt(dir.x * dir.x + dir.y * dir.y);
  float r = atan2(-dir.y, dir.x);
  Matrix4 line_xform = m4_scalar(1);
  line_xform = m4_translate(line_xform, v3(ndc_point_a.x, ndc_point_a.y, 0));
  line_xform = m4_rotate_z(line_xform, r);
  line_xform = m4_translate(line_xform, v3(0, -line_width/2, 0));
  draw_rect_xform(line_xform, v2(length, line_width), color);
}

static
vec4
rect_center_dim(vec2 point, vec2 rect_size) {
  
  vec2 p = point;
  vec2 s = v2_divf(rect_size, 2);
  
  vec4 result = {};
  result.xy = v2(p.x - s.x, p.y - s.y);
  result.zw = v2(p.x + s.x, p.y + s.y);
  
  return result;
}

static
void
render_rect_outline(vec4 rect, vec4 color) {
  
  vec4 r = rect;
  
  render_line(v2(r.x1, r.y1), v2(r.x1, r.y2), color);
  render_line(v2(r.x1, r.y2), v2(r.x2, r.y2), color);
  render_line(v2(r.x2, r.y2), v2(r.x2, r.y1), color);
  render_line(v2(r.x2, r.y1), v2(r.x1, r.y1), color);
  
}


static
void
render_rect_outline_no_ndc(vec4 rect, vec4 color) {
  
  vec4 r = rect;
  
  render_line_no_ndc(v2(r.x1, r.y1), v2(r.x1, r.y2), color);
  render_line_no_ndc(v2(r.x1, r.y2), v2(r.x2, r.y2), color);
  render_line_no_ndc(v2(r.x2, r.y2), v2(r.x2, r.y1), color);
  render_line_no_ndc(v2(r.x2, r.y1), v2(r.x1, r.y1), color);
  
}


static
void
render_outline(vec2 point, vec2 rect_size, vec4 color) {
  
  vec2 p = point;
  vec2 s = v2_divf(rect_size, 2);
  
  render_line(v2(p.x - s.x, p.y - s.y), v2(p.x - s.x, p.y + s.y), color);
  render_line(v2(p.x - s.x, p.y + s.y), v2(p.x + s.x, p.y + s.y), color);
  render_line(v2(p.x + s.x, p.y + s.y), v2(p.x + s.x, p.y - s.y), color);
  render_line(v2(p.x + s.x, p.y - s.y), v2(p.x - s.x, p.y - s.y), color);
}

static
void
render_rect(vec2 pos, vec2 size, vec4 color) {
  m4 rect = identity();
  
  vec2 ndc_pos = ndc_coords(pos);
  float aspect = (f32)window.width / (f32)window.height;
  vec2 adjusted_size = v2(size.x/window.width * aspect, size.y/window.height);
  
  rect = m4_translate(rect, v3(ndc_pos.x, ndc_pos.y, 0));
  draw_rect_xform(rect, adjusted_size, color);
}

static
void
render_entity(Game_state *game_state, Entity *entity, vec4 override_color) {
  
  vec2 ndc_pos = ndc_coords(entity->pos);
  
  vec2 texture_size = v2((f32)game_state->atlas->width, (f32)game_state->atlas->height);
  vec4 sprite_uv = calc_atlas_offset(entity->atlas.current_offset, entity->atlas.size, texture_size);
  vec2 sprite_size_uv = v2_sub(sprite_uv.xy, sprite_uv.zw);
  
  m4 entity_matrix = identity();
  entity_matrix = m4_translate(entity_matrix, v3(ndc_pos.x, ndc_pos.y, 0));
  entity_matrix = m4_translate(entity_matrix, v3(sprite_size_uv.x/2, sprite_size_uv.y/2, 0));
  
  entity_matrix = m4_translate(entity_matrix, v3(-sprite_size_uv.x/2, -sprite_size_uv.y/2, 0));
  entity_matrix = m4_rotate(entity_matrix, v3_z, entity->angle);
  entity_matrix = m4_translate(entity_matrix, v3(sprite_size_uv.x/2, sprite_size_uv.y/2, 0));
  
  entity_matrix = m4_translate(entity_matrix, v3(-sprite_size_uv.x/2, -sprite_size_uv.y/2, 0));
  entity_matrix = m4_scale(entity_matrix, v3(entity->scale, entity->scale, entity->scale));
  entity_matrix = m4_translate(entity_matrix, v3(sprite_size_uv.x/2, sprite_size_uv.y/2, 0));
  
  vec4 color = white_v4;
  f32 threshold = 0.001f;
  
  if (override_color.r <= threshold ||
      override_color.g <= threshold ||
      override_color.b <= threshold ||
      override_color.a <= threshold) {
    color = override_color;
  }
  
  vec2 size = v2_div(entity->atlas.size, texture_size);
  Draw_Quad *quad = draw_rect_xform(entity_matrix, size, color);
  quad->image = game_state->atlas;
  quad->uv = sprite_uv;
}

inline
bool
is_clickable(enum Entity_type type) {
  return (type == Entity_type_rock ||
          type == Entity_type_tree);
}

static
void
setup_entity(Entity *entity, enum Entity_type type) {
  
  entity->type = type;
  entity->scale = 16.0f;
  entity->max_health = 100.0f;
  entity->health = entity->max_health;
  //entity->color = COLOR_WHITE;
  
  switch (type) {
    case Entity_type_null: {
    } break;
    
    case Entity_type_pickaxe: {
      entity->atlas.current_offset = v2(128,31);
      entity->atlas.size           = v2(16,16);
      entity->scale = 32.0f;
    } break;
    
    case Entity_type_player: {
      entity->atlas.current_offset = v2(192,95);
      entity->atlas.size           = v2(8,32);
      entity->scale = 32.0f;
    } break;
    
    case Entity_type_rock: {
      entity->atlas.current_offset = v2(80, 79);
      entity->atlas.size = v2(48,32);
    } break;
    
    
    case Entity_type_item_rock: { 
      entity->atlas.current_offset = v2(16, 143);
      entity->atlas.size = v2(16,16);
    } break;
    
    case Entity_type_item_wood: { 
      entity->atlas.current_offset = v2(0, 143);
      entity->atlas.size = v2(16,16);
    } break;
    
    default: {
      assert(!"qe");
    }
  }
  
}

static
Entity*
entity_create(Game_state *game_state) {
  
  Entity *entity_found = 0;
  for (u32 entity_index = 0; entity_index < game_state->entity_count; entity_index++) {
    Entity *entity = game_state->entity_list + entity_index;
    
    if (!entity->is_valid) {
      entity_found = entity;
      break;
    }
  }
  
  assert(entity_found);
  entity_found->is_valid = true;
  return entity_found;
}

inline
bool
close_by(vec2 pos, vec2 desired_pos, f32 threshold) {
  bool close_by;
  
  f32 dist = v2_length(v2_sub(pos, desired_pos));
  
  close_by = (fabsf(dist) <= threshold);
  
  return close_by;
}

static
void
entity_destroy(Entity *entity) {
  *entity = (Entity){0};
}

static
void
entity_takes_damage(Game_state *game_state, Entity *entity, f32 damage) {
  
  entity->health -= damage;
  
  if (entity->health <= 0.0f) {
    enum Entity_type spawn_type = Entity_type_null;
    
    switch (entity->type) {
      
      case Entity_type_rock: {
        spawn_type = Entity_type_item_rock;
      } break;
      
      case Entity_type_tree: {
        spawn_type = Entity_type_item_wood;
      } break;
      
      default: { assert(!"eq"); }
      
    }
    
    Entity *new_entity = entity_create(game_state);
    setup_entity(new_entity, spawn_type);
    
    new_entity->pos = entity->pos;
    
    entity_destroy(entity);
  }
}


static
void
draw_health_bar(Game_state *game_state, Entity *entity) {
  
  vec2 start = {
    entity->pos.x,
    entity->pos.y - entity->atlas.size.y * 1.2f
  };
  
  f32 width = entity->atlas.size.x * 2;
  f32 height = 6;
  
  vec2 size = { 
    width,
    height
  };
  
  render_outline(start, size, red_v4);
  
  f32 fill_pcent = entity->health / entity->max_health;
  
  fill_pcent += 0.1f; // cuz something is miscalculated
  
  if (fill_pcent < 0)
    fill_pcent = 0;
  
  start = v2(start.x - width / 2, start.y - 1);
  size = v2(lerpf(0, width, fill_pcent), size.y + 1);
  
  render_rect(start, size, green_v4);
}


inline
bool
is_item(enum Entity_type type) {
  return (type > Entity_type_item_start &&
          type < Entity_type_item_end);
}

void SHARED_EXPORT
game_update(f64 delta_time_f64, void *memory, size_t size) {
  
  struct Game_state *game_state = (struct Game_state*)memory;
  
  f32 w = window.width;
  f32 h = window.height;
  
  if (!game_state->is_initialized) {
    
    initialize_arena(&game_state->arena,
                     size - sizeof(Game_state),
                     (u8*)memory + sizeof(Game_state));
    
    game_state->font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
    assert(game_state->font);
    
    u32 entity_count = 1024;
    game_state->entity_list = mem_push_array(&game_state->arena, Entity, entity_count);
    game_state->entity_count = entity_count;
    
    game_state->atlas = load_image_from_disk(STR("../data/atlas.png"), get_heap_allocator());
    assert(game_state->atlas);
    
    game_state->player_entity = entity_create(game_state);
    setup_entity(game_state->player_entity, Entity_type_player);
    game_state->player_entity->pos.y = h/2;
    
    game_state->pickaxe.entity = entity_create(game_state);
    setup_entity(game_state->pickaxe.entity, Entity_type_pickaxe);
    
    // rocks
    {
      u32 rock_count = 5;
      
      for (u32 rock_index = 0; rock_index < rock_count; rock_index++) {
        Entity *entity = entity_create(game_state);
        setup_entity(entity, Entity_type_rock);
        
        entity->pos.x = random_f32(-2*w, 2*w);
        entity->pos.y = h/2;
        //entity->pos.y = random_f32(random, -screen_size.y / 2, screen_size.y / 2);
      }
    }
    
    game_state->is_initialized = true;
  }
  
  f32 delta_time = (f32)delta_time_f64;
  
  game_state->time += delta_time;
  game_state->periodic_time += delta_time;
  if (game_state->periodic_time > TAU)
    game_state->periodic_time -= TAU;
  
  Entity *player = game_state->player_entity;
  
  f32 aspect = (f32)window.width/(f32)window.height;
  vec2 screen_center = v2(w/2, h/2);
  
#define CB_INPUT
  vec2 input_axis = {};
  {
    if (is_key_down('A')) input_axis.x -= 1.0f;
    if (is_key_down('D')) input_axis.x += 1.0f;
    
    if (is_key_just_pressed(KEY_F1)) {
      game_state->show_debug_lines = !game_state->show_debug_lines;
    }
    
    input_axis = v2_normalize(input_axis);
  }
  
  // player entity control
  {
    f32 max_speed = 2048.0f;
    
    vec2 pos_delta = v2_mulf(player->velocity, delta_time);
    player->pos = v2_add(player->pos, pos_delta);
    
    vec2 velocity_delta = v2_mulf(input_axis, delta_time * max_speed);
    player->velocity = v2_add(player->velocity, velocity_delta);
    
    vec2 drag = v2_mulf(player->velocity, -8.0f * delta_time);
    player->velocity = v2_add(player->velocity, drag);
    
    if (v2_length(player->velocity) > max_speed) {
      player->velocity = v2_mulf(v2_normalize(player->velocity), max_speed);
    }
  }
  
  // ground
  {
    render_rect(v2(-window.width, window.height/2-100), v2(window.width*4, 200), hex_to_rgba(0x825f51ff));
  }
  
#define CB_RENDER_OR_NAH
  vec2 player_pos_ndc = ndc_coords(game_state->player_entity->pos);
  for_each_entity(game_state, entity) {
    vec2 size = v2_mulf(entity->atlas.size, 4.0f);
    
    vec4 color = white_v4;
    
    vec2 adjusted_size = {
      (size.x) / w,
      (size.y) / h
    };
    
    f32 aspect = (f32)window.width/(f32)window.height;
    f32 mx = (input_frame.mouse_x/(f32)window.width  * 2.0 - 1.0)*aspect;
    f32 my = input_frame.mouse_y/(f32)window.height * 2.0 - 1.0;
    
    vec4 entity_rect_ndc = rect_center_dim(ndc_coords(entity->pos), adjusted_size);
    bool overlaps = rect_overlaps(entity_rect_ndc, v2(mx, my));
    
    f32 threshold = 0.2f;
    bool close_enough = close_by(ndc_coords(entity->pos), player_pos_ndc, threshold);
    
#define CB_MOUSE_OVER
    
    if (overlaps) {
      game_state->on_entity.entity = entity;
      color = yellow_v4;
      
      if (close_enough && is_clickable(entity->type)) {
        color = green_v4;
        game_state->on_entity.in_range = true;
        draw_health_bar(game_state, entity);
        
        if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
          color = red_v4;
          entity_takes_damage(game_state, entity, 1.0f);
          
          game_state->pickaxe.swinging = true;
          game_state->pickaxe.progress = .0f;
          game_state->pickaxe.swing_speed = PI*2;
          
          game_state->on_entity.pressed = true;
        }
        
      }
    }
    
    if (game_state->show_debug_lines) {
      render_rect_outline_no_ndc(entity_rect_ndc, color);
    }
    
    render_entity(game_state, entity, color);
    
  }
  
#define CB_RANDOM_STUFF
  for_each_entity(game_state, entity) {
    
    if (is_item(entity->type)) {
      entity->pos.y += sin(game_state->time) * 0.1f;
    }
    
  }
  
#define CB_TOOL
  {
    Entity *pickaxe = game_state->pickaxe.entity;
    
    pickaxe->pos = player->pos;
    
    if (game_state->pickaxe.paused) {
      game_state->pickaxe.pause_timer += delta_time;
      
      if (game_state->pickaxe.pause_timer >= 1.0f) {
        game_state->pickaxe.pause_timer = 0;
        game_state->pickaxe.paused = false;
        game_state->pickaxe.swing_speed = -fabs(game_state->pickaxe.swing_speed);
      }
    }
    else if (game_state->pickaxe.swinging) {
      game_state->pickaxe.progress += game_state->pickaxe.swing_speed * delta_time;
      
      if (game_state->pickaxe.progress >= 1.0f) {
        game_state->pickaxe.progress = 1.0f;
        game_state->pickaxe.paused = true;
        game_state->pickaxe.pause_timer = 0;
      }
      
      f32 progress = square(game_state->pickaxe.progress);
      pickaxe->angle = progress * (PI / 4.0f);
    }
    
  }
  
}