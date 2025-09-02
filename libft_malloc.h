#ifndef LIBFT_MALLOC_H
# define LIBFT_MALLOC_H

// https://sourceware.org/glibc/wiki/MallocInternals

# include <stddef.h>

# define TINY_ZONE_TRESHOLD 128
# define SMALL_ZONE_TRESHOLD 1024

// minimum of 8 bytes alignment else respect system requirements for flags
const char MEM_ALIGNMENT = _Alignof(max_align_t) <= 8 ?
    8 : _Alignof(max_align_t);

enum ZONE_TYPE { TINY, SMALL, LARGE };

/* ZONE/BLOCKS metadata */

// [block_metadata][user_data][size_footer]

// [user_data][freed_block_list_t]
typedef struct freed_block_list_s {
    struct freed_block_list_s *next;
    size_t size;
}   freed_block_list_t;

// typedef struct block_metadata_s {
// 	size_t size;
// }	block_metadata_t;

typedef struct zone_metadata_s {
	struct zone_metadata_s *next;
	freed_block_list_t *begin;
	// freed_block_list_t *last;
}	zone_metadata_t;

# define ALIGN_SIZE(size) ((size + alignof(max_align_t) - 1) \
    & ~(alignof(max_align_t) - 1))

/* global allocator structure */

typedef struct allocator_s {
	zone_metadata_t* tiny_zone;
	zone_metadata_t* small_zone;
	zone_metadata_t* large_zone;
}	allocator_t;

/* functions prototypes */

static allocator_t g_allocator = { NULL, NULL, NULL };

void	free(void *ptr);
void	*malloc(size_t size);
void	*realloc(void *ptr, size_t size);

#endif
