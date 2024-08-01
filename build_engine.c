
///
// Build config stuff

#define OOGABOOGA_BUILD_SHARED_LIBRARY 1

#include "oogabooga/oogabooga.c"

///
///
// This is the "engine" part of your game, which will call into your game.dll


typedef void (*Game_Update_Proc)(f64, void*, size_t);
Game_Update_Proc game_update;

Dynamic_Library_Handle dll = 0;

static
FILETIME
win32_get_last_write_time(char* filename) {
  FILETIME result = {};
  
  WIN32_FILE_ATTRIBUTE_DATA file_data;
  if (GetFileAttributesEx(filename, GetFileExInfoStandard, &file_data)) {
    result = file_data.ftLastWriteTime;
  }
  
  return result;
}

void load_game_dll(char **argv) {
  
  // Here we load all the game symbols
  
  if (dll) {
    os_unload_dynamic_library(dll);
  }
  
  string exe_path = STR(argv[0]);
  string exe_dir = get_directory_of(exe_path);
  
  // We need to copy the original and open the copy, so we can recompile the original and then close & replace
  // the copy.
  string dll_path = string_concat(exe_dir, STR("/game.dll"), get_temporary_allocator());
  string used_dll_path = string_concat(exe_dir, STR("/game-in-use.dll"), get_temporary_allocator());
  
  bool ok = os_file_copy(dll_path, used_dll_path, true);
  assert(ok, "Could not copy %s to %s", dll_path, used_dll_path);
  
  dll = os_load_dynamic_library(used_dll_path);
  assert(dll, "Failed loading game dll");
  
  game_update = os_dynamic_library_load_symbol(dll, STR("game_update"));
  assert(game_update, "game is missing game_update()");
  
  log("Loaded game procedures");
}



int entry(int argc, char **argv) {
	
  load_game_dll(argv);
	
	window.title = STR("Minimal Game Example");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0x6495EDff);
  
  string exe_path = STR(argv[0]);
  string exe_dir = get_directory_of(exe_path);
  string dll_path = string_concat(exe_dir, STR("/game.dll"), get_temporary_allocator());
  
	float64 last_time = os_get_current_time_in_seconds();
	
  u32 size_in_mb = 256;
  size_t size_bytes = MB(size_in_mb);
  void *memory = heap_alloc(size_bytes);
  memset(memory, 0, size_bytes);
  
  const float64 fps_limit = 60;
	const float64 min_frametime = 1.0 / fps_limit;
  
  FILETIME dll_last_write_time = {};
  
  while (!window.should_close) {
		
		float64 now = os_get_current_time_in_seconds();
		float64 delta = now - last_time;
		if (delta < min_frametime) {
			os_high_precision_sleep((min_frametime - delta) * 1000.0);
			now = os_get_current_time_in_seconds();
			delta = now - last_time;
		}
#if 0
    if ((int)now != (int)last_time) 
      log("%.2f FPS\n%.2fms", 1.0/(delta), (delta) * 1000);
#endif
    
		last_time = now;
		tm_scope("os_update") {
			os_update(); 
		}
    
		reset_temporary_storage();
		
		game_update(delta, memory, size_bytes);
		
    if (is_key_just_pressed(KEY_F6)) {
      window.should_close = true;
    }
    
    // auto reload
    {
      char *dll = temp_convert_to_null_terminated_string(dll_path);
      FILETIME new_dll_write_time = win32_get_last_write_time(dll);
      
      if (CompareFileTime(&new_dll_write_time, &dll_last_write_time) != 0) {
        load_game_dll(argv);
        
        dll_last_write_time = new_dll_write_time;
      }
    }
    
		os_update(); 
		gfx_update();
	}
  
	return 0;
}