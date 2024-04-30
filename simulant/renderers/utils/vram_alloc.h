#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int vram_alloc_init(void* pool, size_t size);
void vram_alloc_shutdown(void* pool);

void* vram_alloc_malloc(void* pool, size_t size);
void vram_alloc_free(void* pool, void* p);

typedef void (defrag_address_move)(void*, void*, void*);
void vram_alloc_run_defrag(void* pool, defrag_address_move callback,
                           int max_iterations, void* user_data);

size_t vram_alloc_count_free(void* pool);
size_t vram_alloc_count_continuous(void* pool);

void* vram_alloc_next_available(void* pool, size_t required_size);
void* vram_alloc_base_address(void* pool);
size_t vram_alloc_block_count(void* pool);

size_t vram_alloc_pool_size(void* pool);

#ifdef __cplusplus
}
#endif

