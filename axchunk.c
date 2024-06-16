//
// Created by easy on 16.06.24.
//

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "axchunk.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))

/**
 * Same as axc_index, but without bounds checking.
 */
static inline void *axc__index__(axchunk *c, uint64_t i) {
    return (char *) c->items + i * c->width;
}

axchunk *axc_new(uint64_t width) {
    return axc_newSized(width, 8);
}

axchunk *axc_newSized(uint64_t width, uint64_t size) {
    size += !size;
    width += !width;
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
    c->destroy = NULL;
    c->resizeEventHandler = NULL;
    return c;
}

void *axc_destroy(axchunk *c) {
    if (c->destroy) {
        for (char *chunk = c->items; c->len; --c->len) {
            c->destroy(chunk);
            chunk += c->width;
        }
    }
    void *resizeEventArgs = c->resizeEventArgs;
    free(c->items);
    free(c);
    return resizeEventArgs;
}

bool axc_resize(axchunk *c, uint64_t size) {
    size += !size;
    if (size == c->cap)
        return false;
    intptr_t oldItems = (intptr_t) c->items;
    void *items = realloc(c->items, size * c->width);
    if (!items)
        return true;
    ptrdiff_t offset = (intptr_t) items - oldItems;
    c->items = items;
    c->cap = size;
    c->resizeEventHandler(c, offset, c->resizeEventArgs);
    return false;
}

axchunk *axc_swap(axchunk *c, uint64_t i1, uint64_t i2) {
    if (i1 == i2 || i1 >= c->len || i2 >= c->len)
        return c;
    enum {BUFSIZE = 16};
    char buf[BUFSIZE];
    char *chunk1 = axc__index__(c, i1);
    char *chunk2 = axc__index__(c, i2);
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
        axc__quick_memcpy__(buf, chunk1, k);
        axc__quick_memcpy__(chunk1, chunk2, k);
        axc__quick_memcpy__(chunk2, buf, k);
    }
    return c;
}

axchunk *axc_foreach(axchunk *c, bool (*f)(void *, void *), void *arg) {
    char *chunk = c->items;
    for (uint64_t i = 0; i < c->len; ++i) {
        if (!f(chunk, arg)) {
            return c;
        }
        chunk += c->width;
    }
    return c;
}

axchunk *axc_filter(axchunk *c, bool (*f)(const void *, void *), void *arg) {
    const bool shouldDestroy = c->destroy;
    char *chunk = c->items;
    char *filterChunk = c->items;
    for (uint64_t i = 0; i < c->len; ++i) {
        if (f(chunk, arg)) {
            if (chunk != filterChunk)
                axc__quick_memcpy__(filterChunk, chunk, c->width);
            filterChunk += c->width;
        } else if (shouldDestroy) {
            c->destroy(chunk);
        }
        chunk += c->width;
    }
    c->len -= (chunk - filterChunk) / c->width;
    return c;
}

axchunk *axc_clear(axchunk *c) {
    if (c->destroy) {
        for (char *chunk = c->items; c->len; --c->len) {
            c->destroy(chunk);
            chunk += c->width;
        }
    } else {
        c->len = 0;
    }
    return c;
}

axchunk *axc_discard(axchunk *c, uint64_t n) {
    n = c->len - MIN(c->len, n);
    if (c->destroy) {
        for (char *chunk = axc__index__(c, c->len); c->len > n; --c->len)
            c->destroy(chunk -= c->width);
    } else {
        c->len = n;
    }
    return c;
}

