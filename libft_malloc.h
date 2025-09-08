#ifndef LIBFT_MALLOC_H
# define LIBFT_MALLOC_H

// https://sourceware.org/glibc/wiki/MallocInternals

# include <stddef.h>
# include <stdlib.h>

# define TINY_ZONE_TRESHOLD 128
# define SMALL_ZONE_TRESHOLD 2048

// minimum of 8 bytes alignment else respect system requirements for flags
extern const char MEM_ALIGNMENT;

enum ZONE_TYPE { TINY, SMALL, LARGE };

# define ALIGN(size) ((size + _Alignof(max_align_t) - 1) \
    & ~(_Alignof(max_align_t) - 1))

/* chunks metadata */

typedef enum {
    IS_PREV_FREE = 1 << 0,
    RANDOM_FLAG = 1 << 1
}   chunk_metadata_t;

# define CHUNK_META_MASK 0x7

// Data chunk:
//  - allocated = [size & flags][payload][prev_size]
//  - freed = [size & flags][forward and backward pointer][prev_size]
typedef struct chunk_header_s {
    size_t size;
}   chunk_header_t;

typedef struct freed_chunk_header_s {
    size_t size;
    struct freed_chunk_header_s *next;
}   freed_chunk_header_t;

typedef struct chunk_footer_s {
    size_t prev_size;   // same as size inside chunk_header_t
}   chunk_footer_t;

extern const char CHUNK_HEADER_SIZE;
extern const char CHUNK_FOOTER_SIZE;
extern const char MIN_FREED_CHUNK_SIZE;

/* zones metadata */

typedef struct zone_metadata_s {
	struct zone_metadata_s *next;
	freed_chunk_header_t *begin;
	// freed_chunk_list_t *last;
}	zone_metadata_t;

/* global allocator structure */

typedef struct allocator_s {
	zone_metadata_t *tiny_zone;
	zone_metadata_t *small_zone;
	zone_metadata_t *large_zone;
}	allocator_t;

extern allocator_t g_allocator;

/* utils function */

struct zone_info_s {
	zone_metadata_t **zone;
	enum ZONE_TYPE type;
};

struct zone_info_s _get_zone_infos(const size_t size);

/* functions prototypes */

void	free(void *ptr);
void	*malloc(size_t size);
void	*realloc(void *ptr, size_t size);

#endif
