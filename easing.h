/* date = August 5th 2024 8:31 pm */

#ifndef EASING_H
#define EASING_H

static
f32
ease_in_sine(f32 x) {
  return 1 - cosf((x * PI) / 2);
}

static
f32
ease_in_out_expo(f32 x) {
  
  if (x == 0) {
    return 0;
  } else if (x == 1) {
    return 1;
  } else if (x < 0.5) {
    return pow(2, 20 * x - 10) / 2;
  } else {
    return (2 - pow(2, -20 * x + 10)) / 2;
  }
  
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

#endif //EASING_H
