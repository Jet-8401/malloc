#ifndef LIBFT_MALLOC_H
# define LIBFT_MALLOC_H

// https://sourceware.org/glibc/wiki/MallocInternals

# include <stddef.h>
# include <stdlib.h>

# define TINY_ZONE_TRESHOLD 128
# define SMALL_ZONE_TRESHOLD 2048

// Minimum of 8 bytes alignment else respect system requirements, that value is
// set that way to utilize the three last bit of the `size` metadata
// inside chunks for flags.
extern const unsigned char MEM_ALIGNMENT; // [8, sysconf(_SC_PAGESIZE))

// ALIGN macro will return the size aligned to MEM_ALIGNMENT constant
# define ALIGN(size) ((size + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT - 1))
# define ALIGN_TO(size, alignment) ((size + alignment - 1) & ~(alignment - 1))

/* chunks metadata */

typedef enum {
    IS_PREV_FREE = 1 << 0,
    IS_ANCHOR = 1 << 1
}   chunk_metadata_t;

# define CHUNK_META_MASK 0x7

// Data chunk:
//  - allocated = [size & flags][payload][prev_size]
//  - freed = [size & flags][forward and backward pointer][prev_size]
// `size` is the full chunk size not just the user payload
typedef struct chunk_header_s {
    size_t size;
}   chunk_header_t;

// freed_* structures are metadata that apply only for freed chunks
typedef struct freed_chunk_header_s {
    size_t size;
    struct freed_chunk_header_s *next;
    struct freed_chunk_header_s *prev;
}   freed_chunk_header_t;

typedef struct freed_chunk_footer_s {
    size_t prev_size;   // same as size inside chunk_header_t
}   freed_chunk_footer_t;

extern const unsigned char CHUNK_HEADER_SIZE;
extern const unsigned char MIN_FREED_CHUNK_SIZE;

/* zones metadata */

enum ZONE_TYPE { TINY, SMALL, LARGE };

// Zone is a multiple of sysconf(_SC_PAGESIZE).
// In every zone type even LARGE ones an entire zone is one mmap call.

typedef struct zone_metadata_s {
	size_t size;
	struct zone_metadata_s *next;
	freed_chunk_header_t *begin;
	chunk_header_t *top;
}	zone_metadata_t;

extern const unsigned char ALIGNED_ZONE_METADATA;
// `*_ALLOC_SIZE` is the raw size of the minumum to allocate to fit at least 100
// allocations inside a zone.
// They always must be equal or greater than MIN_FREED_CHUNK_SIZE !
extern const size_t TINY_ZONE_ALLOC_SIZE;
extern const size_t SMALL_ZONE_ALLOC_SIZE;

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
