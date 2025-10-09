#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ADDR_LEN 32768

typedef struct address_s {
    void *addr;
    bool is_freed;
}   address_t;

int main() {
    srand(time(NULL));
    address_t *addr[ADDR_LEN];
    memset(addr, 0, sizeof(addr));

    for (int i = 0; i < ADDR_LEN; i++) {
        int size = rand() % 4096 + 1;
        addr[i] = malloc(size);
    }

    for (int i = 0; i < ADDR_LEN * 4; i++) {
        int idx = rand() % ADDR_LEN;
        if (addr[idx] != NULL && !addr[idx]->is_freed) {
            free(addr[idx]->addr);
            addr[idx]->is_freed = true;
        }
    }

    free(malloc(100000));
    free(malloc(34));

    return 0;
}
