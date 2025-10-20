#ifndef LIBFT_MALLOC_H
# define LIBFT_MALLOC_H

// https://sourceware.org/glibc/wiki/MallocInternals

# include <stddef.h>
# include <stdlib.h>
# include <pthread.h>
# include <stdint.h>

# define TINY_ZONE_TRESHOLD 128
# define SMALL_ZONE_TRESHOLD 2048

// Minimum of 8 bytes alignment else respect system requirements, that value is
// set that way to utilize the three last bit of the `size` metadata
// inside chunks for flags.
static const uint8_t MEM_ALIGNMENT = _Alignof(max_align_t) <= 8 ?
    8 : _Alignof(max_align_t);

// ALIGN macro will return the size aligned to MEM_ALIGNMENT constant
# define ALIGN(size) ((size + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT - 1))
# define ALIGN_TO(size, alignment) ((size + alignment - 1) & ~(alignment - 1))

/* chunks metadata */

# define IS_PREV_FREE ((size_t) 1 << 1)
# define CHUNK_META_MASK ((size_t) 0x7)
# define UNMASK(size) (size & ~CHUNK_META_MASK)

// Data chunk:
//  - allocated = [size & flags][payload][prev_size]
//  - freed = [size & flags][forward and backward pointer][prev_size]
// `size` is the full chunk size not just the user payload
typedef struct chunk_header_s {
    size_t size;
}   chunk_header_t;

// freed_* structures are metadata that apply only for freed chunks
typedef struct freed_header_s {
    size_t size;
    struct freed_header_s *next;
    struct freed_header_s *prev;
}   freed_header_t;

typedef struct freed_footer_s {
    size_t prev_size;   // same as size inside chunk_header_t
}   freed_footer_t;

/* zones metadata */

enum ZONE_TYPE { TINY, SMALL, LARGE };

// Zone is a multiple of sysconf(_SC_PAGESIZE).
// In every zone type even LARGE ones an entire zone is one mmap call.

typedef struct zone_metadata_s {
	size_t size;
	struct zone_metadata_s *next;
	freed_header_t *begin;
	chunk_header_t *top;
}	zone_metadata_t;

// typedef struct large_zone_metadata_s {
//     size_t size;
//     struct large_zone_metadata_s *next;
// }   large_zone_meta_t;

// typedef union zone_u {
//     zone_metadata_t std_zone;
//     large_zone_meta_t large_zone;
// }   zone_t;

/* global allocator structure */

typedef struct allocator_s {
	zone_metadata_t *tiny_zone;
	zone_metadata_t *small_zone;
	zone_metadata_t *large_zone;
}	allocator_t;

typedef struct malloc_context_s {
    allocator_t allocator;

    pthread_mutex_t tiny_lock;
	pthread_mutex_t small_lock;
	pthread_mutex_t large_lock;

	const uint8_t ALIGNED_ZONE_METADATA;
	// const uint8_t ALIGNED_LARGE_ZONE_META;
	const uint8_t HEADER_SIZE;
	const uint8_t MIN_CHUNK_SIZE;

	// `*_ALLOC_SIZE` is the raw size of the minumum to allocate to fit at least
	// 100 allocations inside a zone.
    // They always must be equal or greater than MIN_FREED_CHUNK_SIZE !
	const size_t TINY_ZONE_ALLOC_SIZE;
	const size_t SMALL_ZONE_ALLOC_SIZE;
}   mctx_t;

extern mctx_t mctx;

/* utils function */

struct zone_info_s {
	zone_metadata_t **zone;
	enum ZONE_TYPE type;
	pthread_mutex_t *lock;
};

struct zone_info_s get_zone_infos(const size_t size);
void zone_push_back(zone_metadata_t **head, zone_metadata_t *zone);

static inline chunk_header_t *advance_chunk(chunk_header_t *chunk) {
    return (chunk_header_t*) ((char*) chunk + UNMASK(chunk->size));
}

static inline freed_header_t *get_prev_chunk(chunk_header_t *chunk) {
    if (!(chunk->size & IS_PREV_FREE))
        return NULL;
    freed_footer_t *footer = (freed_footer_t*) ((char*) chunk -
        sizeof(freed_footer_t));
    return (freed_header_t*) ((char*) chunk - footer->prev_size);
}

static inline void write_footer(freed_header_t *chunk) {
    size_t raw_size = UNMASK(chunk->size);
    freed_footer_t *footer = (freed_footer_t*) ((char*) chunk + raw_size -
        sizeof(freed_footer_t));
    footer->prev_size = raw_size;
}

/* functions prototypes */

void	free(void *ptr);
void	*malloc(size_t size);
void	*realloc(void *ptr, size_t size);
void    *calloc(size_t nmemb, size_t size);

void    show_alloc_mem();

#endif
