const float64 fps_limit = 60;
const float64 min_frametime = 1.0 / fps_limit;


float64 now = os_get_current_time_in_seconds();
float64 delta = now - last_time;
if (delta < min_frametime) {
  os_high_precision_sleep((min_frametime-delta)*1000.0);
  now = os_get_current_time_in_seconds();
  delta = now - last_time;
}
last_time = now;
tm_scope("os_update") {
  os_update(); 
}

if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(delta), (delta)*1000);