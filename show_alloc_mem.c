#include "libft_malloc.h"
#include <unistd.h>

static const char HEXA_BASE[] = "0123456789abcdef";
static const char DEC_BASE[] = "0123456789";

static size_t _ft_strlen(const char *s) {
	size_t	i = 0;

	while (*(s + i++));
	return (i - 1);
}

static void	_ft_putnbr_base(
    ssize_t n,
    const char *base,
    const ssize_t base_len,
    const char *prefix
) {
	char	c;

	if (n >= base_len)
		_ft_putnbr_base(n / base_len, base, base_len, prefix);
	else if (prefix)
        write(1, prefix, _ft_strlen(prefix));
	c = base[n % base_len];
	write(1, &c, 1);
}

#define print_hex(n) _ft_putnbr_base(n, HEXA_BASE, 16, "0x");
#define print_dec(n) _ft_putnbr_base(n, DEC_BASE, 10, NULL);

static void _show_allocations_in_zone(zone_metadata_t *zone) {
    chunk_header_t *it = (void*) (
        (uint8_t*) zone + mctx.ALIGNED_ZONE_METADATA
    );

    for (; it != zone->top; it = ADVANCE_CHUNK(it)) {
        if (it->size & IS_FREE)
            continue;

        print_hex((size_t) it);
        write(1, " : ", 3);
        // print_dec(GET_RAW_SIZE(it));
        print_dec(it->size);
        write(1, " bytes\n", 7);
    }
}

static void _show_zones_allocations(zone_metadata_t *head, const char *type) {
    zone_metadata_t *zone_it;

    // put the header for the zone
    write(1, type, _ft_strlen(type));
    write(1, " : ", 3);
    if (head == NULL)
        write(1, "Nothing allocated...", 20);
    else
        print_hex((size_t) head);
    write(1, "\n", 1);

    for (zone_it = head; zone_it != NULL; zone_it = zone_it->next) {
        _show_allocations_in_zone(zone_it);
    }
}

void show_alloc_mem() {
    _show_zones_allocations(mctx.allocator.tiny_zone, "TINY");
    _show_zones_allocations(mctx.allocator.small_zone, "SMALL");
    _show_zones_allocations(mctx.allocator.large_zone, "LARGE");
}
