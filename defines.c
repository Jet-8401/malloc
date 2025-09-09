#include "libft_malloc.h"

allocator_t g_allocator = { NULL, NULL, NULL };

const unsigned char CHUNK_HEADER_SIZE = ALIGN(sizeof(chunk_header_t));
const unsigned char CHUNK_FOOTER_SIZE = ALIGN(sizeof(chunk_footer_t));
const unsigned char MIN_FREED_CHUNK_SIZE = CHUNK_HEADER_SIZE +
    ALIGN(sizeof(chunk_footer_t));
const unsigned char MEM_ALIGNMENT = _Alignof(max_align_t) <= 8 ?
    8 : _Alignof(max_align_t);
