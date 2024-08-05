#define OOGABOOGA_LINK_EXTERNAL_INSTANCE 1

#include "oogabooga/oogabooga.c"

#include "game.h"

typedef int32_t i32;

#define UI_BUILDINGS 1

const vec4 no_color_override = {0,0,0,0};
const i32 tile_width = 8;

#define array_count(array) (sizeof(array) / sizeof((array)[0]))

inline
bool 
almost_equals(f32 a, f32 b, f32 epsilon) {
  return fabs(a - b) <= epsilon;
}

inline
bool
exp_interpolate(f32* value, f32 target, f32 delta_t, f32 rate) {
	
  *value += (target - *value) * (1.0f - powf(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f)) {
		*value = target;
		return true;
	}
	return false;
}

static
void
v2_exp_interpolate(vec2 *value, vec2 target, f32 delta_t, f32 rate) {
  exp_interpolate(&value->x, target.x, delta_t, rate);
  exp_interpolate(&value->y, target.y, delta_t, rate);
}

inline
i32 world_pos_to_tile_pos(f32 world_pos) {
	return roundf(world_pos / (f32)tile_width);
}

inline
m4
identity() {
  m4 result = m4_scalar(1.0);
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
m4
apply_camera(m4 matrix, Camera *camera) {
  
  m4 result = identity();
  
  m4 camera_matrix = identity();
  
  vec2 ndc_cam = ndc_coords(camera->pos);
  
  camera_matrix = m4_translate(camera_matrix, v3(-ndc_cam.x, -ndc_cam.y, 0));
  
  
  camera_matrix = m4_rotate(camera_matrix, v3_z, -camera->angle);
  camera_matrix = m4_scale(camera_matrix, v3(camera->zoom, camera->zoom, 1.0f));
  
  result = m4_mul(camera_matrix, matrix);
  
  return result;
}

static
m4 
inverse_camera_matrix(Camera *camera) {
  
  m4 camera_matrix = identity();
  
  vec2 ndc_cam = ndc_coords(camera->pos);
  
  camera_matrix = m4_translate(camera_matrix, v3(-ndc_cam.x, -ndc_cam.y, 0));
  camera_matrix = m4_rotate(camera_matrix, v3_z, -camera->angle);
  camera_matrix = m4_scale(camera_matrix, v3(camera->zoom, camera->zoom, 1.0f));
  
  m4 inverse_camera = m4_inverse(camera_matrix);
  
  return inverse_camera;
}

static
vec2 
ndc_to_world(vec2 ndc_coords, m4 inverse_camera_matrix) {
  vec4 ndc_pos = v4(ndc_coords.x, ndc_coords.y, 0.0f, 1.0f);
  vec4 world_pos = m4_transform(inverse_camera_matrix, ndc_pos);
  
  return v2(world_pos.x, world_pos.y);
}

static
f32
ease_out_bounce(f32 progress) {
  f32 n1 = 7.5625f;
  f32 d1 = 2.75f;
  
  f32 x = progress;
  
  if (x < 1.0f / d1) {
    return n1 * x * x;
  } 
  else if (x < 2.0f / d1) {
    x -= 1.5f / d1;
    return n1 * x * x + 0.75f;
  } 
  else if (x < 2.5f / d1) {
    x -= 2.25f / d1;
    return n1 * x * x + 0.9375f;
  } 
  else {
    x -= 2.625f / d1;
    return n1 * x * x + 0.984375f;
  }
}

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

static
void
render_line_no_ndc(vec2 point, vec2 end_point, vec4 color, Camera *camera) {
  
  // no ndc - don't apply ndc
  
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
  
  m4 final_matrix = apply_camera(line_xform, camera);
  
  draw_rect_xform(final_matrix, v2(length, line_width), color);
}

static
void
render_line(vec2 point, vec2 end_point, vec4 color, Camera *camera) {
  
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
  
  m4 final_matrix = apply_camera(line_xform, camera);
  
  draw_rect_xform(final_matrix, v2(length, line_width), color);
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
render_rect_outline(vec4 rect, vec4 color, Camera *camera) {
  vec4 r = rect;
  
  render_line(v2(r.x1, r.y1), v2(r.x1, r.y2), color, camera);
  render_line(v2(r.x1, r.y2), v2(r.x2, r.y2), color, camera);
  render_line(v2(r.x2, r.y2), v2(r.x2, r.y1), color, camera);
  render_line(v2(r.x2, r.y1), v2(r.x1, r.y1), color, camera);
  
}

static
void
render_rect_outline_no_ndc(vec4 rect, vec4 color, Camera *camera) {
  // no ndc - don't apply ndc
  vec4 r = rect;
  
  render_line_no_ndc(v2(r.x1, r.y1), v2(r.x1, r.y2), color, camera);
  render_line_no_ndc(v2(r.x1, r.y2), v2(r.x2, r.y2), color, camera);
  render_line_no_ndc(v2(r.x2, r.y2), v2(r.x2, r.y1), color, camera);
  render_line_no_ndc(v2(r.x2, r.y1), v2(r.x1, r.y1), color, camera);
}


static
void
render_outline(vec2 point, vec2 rect_size, vec4 color, Camera *camera) {
  
  vec2 p = point;
  vec2 s = v2_divf(rect_size, 2);
  
  render_line(v2(p.x - s.x, p.y - s.y), v2(p.x - s.x, p.y + s.y), color, camera);
  render_line(v2(p.x - s.x, p.y + s.y), v2(p.x + s.x, p.y + s.y), color, camera);
  render_line(v2(p.x + s.x, p.y + s.y), v2(p.x + s.x, p.y - s.y), color, camera);
  render_line(v2(p.x + s.x, p.y - s.y), v2(p.x - s.x, p.y - s.y), color, camera);
}


static
void
render_rect(vec2 pos, vec2 size, vec4 color, Camera *camera) {
  m4 rect = identity();
  
  vec2 ndc_pos = ndc_coords(pos);
  
  rect = m4_translate(rect, v3(ndc_pos.x, ndc_pos.y, 0));
  
  m4 final_matrix = apply_camera(rect, camera);
  
  draw_rect_xform(final_matrix, size, color);
}

static
void
render_entity(Gfx_Image *atlas, Entity *entity, Camera *camera, vec4 override_color) {
  
  vec2 ndc_pos = ndc_coords(entity->pos);
  
  vec2 texture_size = v2((f32)atlas->width, (f32)atlas->height);
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
  
  m4 final_matrix = apply_camera(entity_matrix, camera);
  
  vec4 color = entity->color;
  f32 threshold = 0.001f;
  
  if (override_color.r >= threshold ||
      override_color.g >= threshold ||
      override_color.b >= threshold ||
      override_color.a >= threshold) {
    color = override_color;
  }
  
  vec2 size = v2_div(entity->atlas.size, texture_size);
  Draw_Quad *quad = draw_rect_xform(final_matrix, size, color);
  quad->image = atlas;
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
  entity->color = COLOR_WHITE;
  
  switch (type) {
    case Entity_type_null: {
    } break;
    
    case Entity_type_building_luberjack_hut: {
      entity->atlas.current_offset = v2(48,223);
      entity->atlas.size           = v2(80,48);
    } break;
    
    case Entity_type_pickaxe: {
      entity->atlas.current_offset = v2(128,47);
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
  
  vec2 adjusted_size = {
    (size.x) / window.width,
    (size.y) / window.height
  };
  
  render_outline(start, size, red_v4, &game_state->camera);
  
  f32 fill_pcent = entity->health / entity->max_health;
  
  if (fill_pcent < 0)
    fill_pcent = 0;
  
  start = v2(start.x - width / 2, start.y - 1);
  size = v2(lerpf(0, adjusted_size.x * 2, fill_pcent), adjusted_size.y);
  
  render_rect(start, size, green_v4, &game_state->camera);
}


inline
bool
is_item(enum Entity_type type) {
  return (type > Entity_type_item_start &&
          type < Entity_type_item_end);
}

static
void
format_buffer(char* buffer, char* text, ...) {
  
  va_list args;
  va_start(args, text);
  
  vsnprintf((char*)buffer, 256, text, args);
}

static
void
info(Gfx_Font *font, vec2 pos, char *text, ...) {
  
  char buffer[256]; 
  va_list args;
  va_start(args, text);
  
  vsnprintf((char*)buffer, 256, text, args);
  
  vec2 ndc_pos = ndc_coords(pos);
  draw_text(font, STR(buffer), 64, ndc_pos, v2(0.001, 0.001), COLOR_BLACK);
}


static
void
render_text(Gfx_Font *font, char *text, vec2 pos) {
  vec2 ndc_pos = ndc_coords(pos);
  draw_text(font, STR(text), 128, ndc_pos, v2(0.001, 0.001), COLOR_BLACK);
}

static
void
play_sound(enum Sound_effect sound_id) {
  
  switch (sound_id) {
    case Sound_effect_pickaxe_hit: {
      
      char *path_list[] = {
        "../data/sounds/pick_1_wet.wav",
        "../data/sounds/pick_1_wet2.wav",
        "../data/sounds/pick_1_wet3.wav",
      };
      
      u32 random_index = get_random_int_in_range(0, array_count(path_list) - 1);
      
      play_one_audio_clip(STR(path_list[random_index]));
    } break;
  }
  
}

static
void
apply_gravity(vec2 *velocity, f32 delta_time) {
  velocity->y += 9.8f * 100.0f * delta_time; 
}

static
void
apply_velocity(vec2 *in_pos, vec2 *in_velocity, vec2 input_axis, f32 delta_time) {
  
  f32 max_speed = 3000.0f;
  
  vec2 velocity = *in_velocity;
  vec2 pos = *in_pos;
  
  vec2 pos_delta = v2_mulf(velocity, delta_time);
  pos = v2_add(pos, pos_delta);
  
  vec2 velocity_delta = v2_mulf(input_axis, delta_time * max_speed);
  velocity = v2_add(velocity, velocity_delta);
  
  vec2 drag = v2_mulf(velocity, -8.0f * delta_time);
  velocity = v2_add(velocity, drag);
  
  if (v2_length(velocity) > max_speed) {
    velocity = v2_mulf(v2_normalize(velocity), max_speed);
  }
  
  *in_velocity = velocity;
  *in_pos = pos;
}

static
vec2
calc_mouse_ndc() {
  f32 aspect = (f32)window.width/(f32)window.height;
  f32 mx = (input_frame.mouse_x / (f32)window.width  * 2.0 - 1.0)*aspect;
  f32 my = input_frame.mouse_y  / (f32)window.height * 2.0 - 1.0;
  
  return v2(mx, my);
}

static
vec2
calc_mouse_world_pos(Camera *camera) {
  vec2 mouse_ndc = calc_mouse_ndc();
  
  m4 inv_camera_matrix = inverse_camera_matrix(camera);
  vec2 mouse_world = ndc_to_world(mouse_ndc, inv_camera_matrix);
  
  return mouse_world;
}

void SHARED_EXPORT
game_update(f64 delta_time_f64, void *memory, size_t size) {
  
  struct Game_state *game_state = (struct Game_state*)memory;
  
  size_t sz = sizeof(Game_state);
  
  f32 w = window.width;
  f32 h = window.height;
  
  window.clear_color = hex_to_rgba(0x193650ff);
  
  if (!game_state->is_initialized) {
    
    initialize_arena(&game_state->arena,
                     size - sizeof(Game_state),
                     (u8*)memory + sizeof(Game_state));
    
    Allocator heap = get_heap_allocator();
    
    game_state->font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), heap);
    assert(game_state->font);
    
    u32 entity_count = 1024;
    game_state->entity_list = mem_push_array(&game_state->arena, Entity, entity_count);
    game_state->entity_count = entity_count;
    
    game_state->atlas = load_image_from_disk(STR("../data/atlas.png"), heap);
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
    
    // particles
    {
      u32 max_particles = 500;
      game_state->particle_list = mem_push_array(&game_state->arena, Particle, max_particles);
      
      for (u32 i = 0; i < max_particles; i += 1) {
        Particle *particle = game_state->particle_list + game_state->particle_count++;
        
        *particle = (Particle){0};
      }
    }
    
    game_state->camera.zoom = 1.0f;
    game_state->ui_camera.zoom = 1.0f;
    
    game_state->is_initialized = true;
  }
  
  f32 delta_time = (f32)delta_time_f64;
  
  //delta_time /= 4.0f;
  
  game_state->time += delta_time;
  game_state->periodic_time += delta_time;
  if (game_state->periodic_time > TAU)
    game_state->periodic_time -= TAU;
  
  Entity *player = game_state->player_entity;
  
  f32 aspect = (f32)window.width/(f32)window.height;
  vec2 screen_center = v2(w/2, h/2);
  
  Camera *camera = &game_state->camera;
  Camera *ui_camera = &game_state->ui_camera;
  Gfx_Image *atlas = game_state->atlas;
  
#define CB_INPUT
  vec2 input_axis = {};
  {
    if (is_key_down('A')) input_axis.x -= 1.0f;
    if (is_key_down('D')) input_axis.x += 1.0f;
    if (is_key_down('S')) input_axis.y -= 1.0f;
    if (is_key_down('W')) input_axis.y += 1.0f;
    
    if (is_key_down('Q')) camera->angle += 0.01f;
    if (is_key_down('E')) camera->angle -= 0.01f;
    
    if (is_key_down('Z')) camera->zoom += 0.01f;
    if (is_key_down('X')) camera->zoom -= 0.01f;
    
    if (is_key_just_pressed('B')) game_state->ui_building.show = !game_state->ui_building.show;
    
    if (is_key_down('C')) {
      *camera = (Camera){0};
      camera->zoom = 1.0f;
    }
    
    if (is_key_just_pressed(KEY_F1)) {
      game_state->show_debug_lines = !game_state->show_debug_lines;
    }
    
    input_axis = v2_normalize(input_axis);
  }
  
#define CB_PLAYER_MOVE
  {
    apply_velocity(&player->pos, &player->velocity, input_axis, delta_time);
  }
  
  // ground
  if (0)
  {
    render_rect(v2(-window.width, window.height/2-100), v2(window.width*4, 200), hex_to_rgba(0x825f51ff), camera);
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
    
    vec2 mouse_world = calc_mouse_world_pos(camera);
    
    vec2 entity_ndc = ndc_coords(entity->pos);
    vec4 entity_rect_ndc = rect_center_dim(entity_ndc, adjusted_size);
    
    bool overlaps = rect_overlaps(entity_rect_ndc, mouse_world);
    
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
        
        if (is_key_down(MOUSE_BUTTON_LEFT)) {
          color = red_v4;
          
#define CB_SWING_START
          if (!game_state->pickaxe.swinging) {
            game_state->pickaxe.swinging = true;
            game_state->pickaxe.hit_entity = entity;
            
            f32 dist = (ndc_coords(entity->pos)).x - (player_pos_ndc).x;
            game_state->pickaxe.left = (dist > 0) ? true : false;
            
            entity->shake_before_pos = entity->pos;
            entity_takes_damage(game_state, entity, 1.0f);
          }
          
          game_state->on_entity.pressed = true;
        }
        
      }
    }
    
    if (game_state->show_debug_lines) {
      render_rect_outline_no_ndc(entity_rect_ndc, color, camera);
      render_line_no_ndc(mouse_world, v2_add(mouse_world, v2(0.01, 0.0)), color, camera);
    }
    
    render_entity(atlas, entity, camera, color);
  }
  
#define CB_RANDOM_STUFF
  for_each_entity(game_state, entity) {
    
    if (is_item(entity->type)) {
      entity->pos.y += sin(game_state->time) * 0.1f;
    }
    
    if (entity->shaking) {
      
      entity->shake_timer -= delta_time;
      
      if (entity->shake_timer < 0.0f) {
        entity->pos = entity->shake_before_pos;
        entity->shaking = false;
      }
      
      entity->pos.y += sin(game_state->time * 50.0f) * 0.9f;
      entity->pos.x += sin(game_state->time * 10.0f) * 0.1f;
    }
  }
  
#define CB_PARTICLE
  
  {
    i32 spawn_new_particles = 0;
    
    if (game_state->pickaxe.hit) {
      spawn_new_particles = 1;
    }
    
    for (u32 particle_index = 0; particle_index < game_state->particle_count; particle_index++) {
      
      Particle *particle = game_state->particle_list + particle_index;
      
      particle->life_left -= delta_time;
      
      if (particle->life_left > 0) {
        
        apply_gravity(&particle->velocity, delta_time);
        
        vec2 velocity_delta = v2_mulf(particle->velocity, delta_time);
        particle->pos       = v2_sub(particle->pos, velocity_delta);
        particle->color.a  -= 1.5f * delta_time;
        
        Entity draw_entity = {};
        draw_entity.atlas.current_offset = v2(16, 159);
        draw_entity.atlas.size = v2(16,16);
        
        draw_entity.color = particle->color;
        draw_entity.pos = particle->pos;
        draw_entity.scale = 2.0f;
        
        if (particle->pos.y > h/2) 
          render_entity(atlas, &draw_entity, camera, no_color_override);
      }
      else {
        if (spawn_new_particles-- > 0) {
          
          Entity *entity = game_state->pickaxe.hit_entity;
          
          f32 rand_offset = random_f32(-10, 10);
          f32 rand_color  = random_f32(0.0, 1);
          
          particle->pos = v2_add(entity->pos, v2(rand_offset, rand_offset));
          particle->color = v4(1.0f, rand_color, 0, 1.0);
          particle->life_left = 1.0f;
          
          f32 min_angle = PI;
          f32 max_angle = PI*2;
          
          f32 random_angle = random_f32(min_angle, max_angle);
          
          f32 speed = 400.0f;
          
          particle->velocity.x = cos(random_angle) * speed;
          particle->velocity.y = sin(random_angle) * speed;
        }
      }
      
    }
    
  }
  
#define CB_CAMERA
  {
    v2_exp_interpolate(&game_state->camera.pos, player->pos, delta_time, 5.0f);
  }
  
#define CB_UI_BUILD
  if (game_state->ui_building.show) {
    
    if (is_key_just_pressed(KEY_ESCAPE)) {
      game_state->ui_building.show = false;
      consume_key_just_pressed(KEY_ESCAPE);
    }
    
    vec4 bg_color = white_v4;
    bg_color.a = 0.5f;
    render_rect(v2(-w,-h), v2(w, h), bg_color, ui_camera);
    
    enum Entity_type type = Entity_type_building_luberjack_hut;
    
    Entity building = {};
    setup_entity(&building, type);
    
    vec2 adjusted_size = {
      (building.atlas.size.x * 4) / w,
      (building.atlas.size.y * 4) / h
    };
    
    vec2 ndc = ndc_coords(building.pos);
    vec4 building_rect_ndc = rect_center_dim(ndc, adjusted_size);
    
    vec2 mouse_world = calc_mouse_world_pos(ui_camera);
    
    bool overlaps = rect_overlaps(building_rect_ndc, mouse_world);
    
    vec4 color = white_v4;
    
    if (overlaps) {
      color = yellow_v4;
    }
    
    if (overlaps && is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
      consume_key_just_pressed(MOUSE_BUTTON_LEFT);
      game_state->ui_building.selected_type = type;
      game_state->ui_building.show = false;
      
      game_state->grid.show = true;
    }
    
    render_entity(atlas, &building, ui_camera, color);
    
    if (game_state->show_debug_lines) {
      render_rect_outline_no_ndc(building_rect_ndc, green_v4, ui_camera);
    }
  }
  
#define CB_GRID
  if (game_state->grid.show) {
    
    if (is_key_just_pressed(KEY_ESCAPE)) {
      game_state->grid.show = false;
      consume_key_just_pressed(KEY_ESCAPE);
    }
    
    vec2 rect_size = v2(64, 64);
    rect_size.x *= aspect;
    vec4 color = white_v4;
    color.a = 0.1;
    
    vec2 adjusted_size = {
      (rect_size.x) / w,
      (rect_size.y) / h
    };
    
    game_state->grid.pressed = false;
    
    for (i32 y = -10; y < 10; y++) {
      for (i32 x = -10; x < 10; x++) {
        vec2 pos = v2(rect_size.x/2 * x, rect_size.y/2 * y);
        
        vec2 ndc = ndc_coords(pos);
        vec4 entity_rect_ndc = rect_center_dim(ndc, adjusted_size);
        
        vec2 mouse_world = calc_mouse_world_pos(camera);
        
        bool overlaps = rect_overlaps(entity_rect_ndc, mouse_world);
        
        if (overlaps) {
          
          game_state->grid.pos = pos;
          game_state->grid.overlaps = true;
          
          vec4 rect_color = color;
          
          if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
            rect_color = green_v4;
            game_state->grid.pressed = true;
            game_state->grid.show = false;
            
#define CB_BUILDING_NEW
            Entity *building = entity_create(game_state);
            setup_entity(building, game_state->ui_building.selected_type);
            
            building->pos = pos;
          }
          
          vec2 rect_start_pos = {
            pos.x - rect_size.x/4,
            pos.y - rect_size.y/4
          };
          
          render_rect(rect_start_pos, adjusted_size, rect_color, camera);
        }
        else {
          render_rect_outline_no_ndc(entity_rect_ndc, color, camera);
        }
        
      }
    }
    
  }
  
#define CB_TOOL
  {
    Entity *pickaxe = game_state->pickaxe.entity;
    
    pickaxe->pos = player->pos;
    
    if (game_state->pickaxe.swinging) {
      
      // a - sin(a), (1 - cos(a)) * 1.5f)
      
      bool left = game_state->pickaxe.left;
      
      f32 side = 1; // default left;
      
      if (!left)
        side = -1;
      
      f32 a = game_state->pickaxe.step;
      
      a += delta_time * 10.0f;
      
      f32 max_a = 6.0f;
      f32 progress = a / max_a;
      f32 t = ease_out_bounce(progress);
      f32 p = t * max_a;
      
      f32 y_scale = 1.5f; // how high y will go
      f32 f_scale = 20.0f; // function scale
      
      f32 x = p - sin(p);
      f32 y = (1 - cos(p)) * y_scale;
      
      x *= f_scale * side;
      y *= f_scale;
      
      f32 max_angle   = PI/3;
      f32 start_angle = -PI/2;
      
      if (!left) {
        f32 temp = max_angle;
        max_angle = start_angle;
        start_angle = temp;
      }
      
      f32 angle = lerpf(start_angle, max_angle, t);
      pickaxe->angle = angle;
      
      if (!game_state->pickaxe.first_hit && t >= .7) {
        game_state->pickaxe.first_hit = true;
        play_sound(Sound_effect_pickaxe_hit);
      }
      
      game_state->pickaxe.hit = (t >= .9) ? false : true;
      
      //printf("%f\n", t);
      
      if (a >= max_a) {
        a = 0.3;
        pickaxe->angle = -PI;
        game_state->pickaxe.swinging = 0;
        game_state->pickaxe.hit = false;
        game_state->pickaxe.first_hit = false;
      }
      
      f32 offset = -50 * side;
      
      game_state->pickaxe.step = a;
      pickaxe->pos = v2(player->pos.x + x + offset, player->pos.y + y);
      
      if (!game_state->pickaxe.hit_entity->shaking && progress > 0.3f) {
        game_state->pickaxe.hit_entity->shaking = true;
        game_state->pickaxe.hit_entity->shake_timer = 0.001f;
      }
    }
  }
  
}