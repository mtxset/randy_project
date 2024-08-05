/* date = July 31st 2024 11:32 am */

#ifndef MEMORY_H
#define MEMORY_H


typedef struct Memory_arena {
  u8* base;
  size_t size;
  size_t used;
  
  u32 temp_count;
} Memory_arena;

typedef struct Temp_memory {
  Memory_arena* arena;
  size_t used;
} Temp_memory;

inline
Temp_memory
begin_temp_memory(Memory_arena* arena) {
  Temp_memory result;
  
  result.arena = arena;
  result.used = arena->used;
  arena->temp_count++;
  
  return result;
}

inline
void
end_temp_memory(Temp_memory temp_memory) {
  Memory_arena* arena = temp_memory.arena;
  assert(arena->used >= temp_memory.used);
  arena->used = temp_memory.used;
  assert(arena->temp_count > 0);
  arena->temp_count--;
}

inline
void 
initialize_arena(Memory_arena* arena, size_t size, void* base) {
  arena->size = size;
  arena->base = (u8*)base;
  arena->used = 0;
  arena->temp_count = 0;
}

inline
void* 
mem_push_size_(Memory_arena* arena, size_t size) {
  assert((arena->used + size) <= arena->size);
  
  void* result = arena->base + arena->used;
  arena->used += size;
  
  return result;
}

#define mem_push_struct(arena,type)      (type *)mem_push_size_(arena, sizeof(type))
#define mem_push_array(arena,type,count) (type *)mem_push_size_(arena, (count) * sizeof(type))
#define mem_push_size(arena,size)       mem_push_size_(arena, size)

#define mem_zero_struct(instance) mem_zero_size_(sizeof(instance), &(instance))

inline
void
sub_arena(Memory_arena* result, Memory_arena *arena, size_t size) {
  result->size = size;
  result->base = (u8*)mem_push_size(arena, size);
  result->used = 0;
  result->temp_count = 0;
}

#endif //MEMORY_H
