//
// Created by easy on 16.06.24.
//

#include "axchunk.h"

static inline void *axc_index(axchunk *c, uint64_t i) {
    return (char *) c->items + i * c->width;
}

axchunk *axc_new(uint64_t width) {
    axchunk *c = malloc(sizeof *c);
    if (c)
        c->items = malloc(8 * width);
    if (!c || !c->items) {
        free(c);
        return NULL;
    }
    c->len = 0;
    c->cap = 8;
    c->width = width;
    return c;
}

axchunk *axc_newSized(uint64_t width, uint64_t size) {
    axchunk *c = malloc(sizeof *c);
    if (c)
        c->items = malloc(size * width);
    if (!c || !c->items) {
        free(c);
        return NULL;
    }
    c->len = 0;
    c->cap = size;
    c->width = width;
    return c;
}

bool axc_resize(axchunk *c, uint64_t size) {
    size = size > 1 ? size : 1;
    void *items = realloc(c->items, size * c->width);
    if (!items)
        return true;
    c->items = items;
    c->cap = size;
    return false;
}

axchunk *axc_swap(axchunk *c, uint64_t i1, uint64_t i2) {
    if (i1 == i2 || i1 >= c->len || i2 >= c->len)
        return c;
    enum {BUFSIZE = 16};
    char buf[BUFSIZE];
    char *chunk1 = axc_index(c, i1);
    char *chunk2 = axc_index(c, i2);
    uint64_t k = c->width;
    while (k >= BUFSIZE) {
        memcpy(buf, chunk1, BUFSIZE);
        memcpy(chunk1, chunk2, BUFSIZE);
        memcpy(chunk2, buf, BUFSIZE);
        chunk1 += BUFSIZE;
        chunk2 += BUFSIZE;
        k -= BUFSIZE;
    }
    if (k) {
        memcpy(buf, chunk1, k);
        memcpy(chunk1, chunk2, k);
        memcpy(chunk2, buf, k);
    }
    return c;
}

