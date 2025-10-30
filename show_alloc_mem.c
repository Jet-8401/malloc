#include "libft/libft.h"
#include "libft_malloc.h"
#include <unistd.h>

static const char HEXA_BASE[] = "0123456789abcdef";
static const char DEC_BASE[] = "0123456789";

static void	_ft_putnbr_base(
    const ssize_t n,
    const char *base,
    const ssize_t base_len,
    const char *prefix
) {
	char	c;

	if (n >= base_len)
		_ft_putnbr_base(n / base_len, base, base_len, prefix);
	else if (prefix)
        ft_putstr_fd(prefix, 1);
	c = base[n % base_len];
	ft_putchar_fd(c, 1);
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
        ft_putstr_fd(" : ", 1);
        print_dec(GET_RAW_SIZE(it));
        ft_putendl_fd(" bytes", 1);
    }
}

static void _show_zones_allocations(zone_metadata_t *head, const char *type) {
    zone_metadata_t *zone_it;

    // put the header for the zone
    ft_putstr_fd(type, 1);
    ft_putstr_fd(" : ", 1);
    if (head == NULL)
        ft_putstr_fd("Nothing allocated...", 1);
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
