#ifndef LIBFT_MALLOC_H
# define LIBFT_MALLOC_H

# include <stddef.h>
// # include <stdlib.h>
# include <stdalign.h>

# define TINY_ZONE_TRESHOLD 128
# define SMALL_ZONE_TRESHOLD 1024

const int TINY_ZONE_ALLOC = TINY_ZONE_TRESHOLD * 100;
const int SMALL_ZONE_ALLOC = SMALL_ZONE_TRESHOLD * 100;

enum ZONE_TYPE {
	TINY,
	SMALL,
	LARGE
};

/* ZONE/BLOCKS metadata */

typedef struct freed_block_list_s {
    struct freed_block_list_s *next;
    size_t size;
}   freed_block_list_t;

typedef struct zone_metadata_s {
	struct zone_metadata_s *next;
	freed_block_list_t *begin;
}	zone_metadata_t;

const size_t ALIGNED_METADATA_SIZE =
    (sizeof(zone_metadata_t) + alignof(max_align_t) - 1) &
    ~(alignof(max_align_t) - 1);

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
