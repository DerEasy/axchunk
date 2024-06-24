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
#define MAX(x,y) ((x) > (y) ? (x) : (y))

static void *(*malloc_)(size_t) = malloc;
static void *(*realloc_)(void *, size_t) = realloc;
static void (*free_)(void *) = free;

/**
 * Same as axc_index, but without bounds checking.
 */
static inline void *axc__index__(axchunk *c, uint64_t i) {
    return (char *) c->chunks + i * c->width;
}

void axc_memoryfn(void *(*malloc_fn)(size_t), void *(*realloc_fn)(void *, size_t), void (*free_fn)(void *)) {
    malloc_ = malloc_fn ? malloc_fn : malloc;
    realloc_ = realloc_fn ? realloc_fn : realloc;
    free_ = free_fn ? free_fn : free;
}

axchunk *axc_new(uint64_t width) {
    return axc_newSized(width, 7);
}

axchunk *axc_newSized(uint64_t width, uint64_t size) {
    size += !size;
    width += !width;
    axchunk *c = malloc_(sizeof *c);
    if (c)
        c->chunks = malloc_(size * width);
    if (!c || !c->chunks) {
        free_(c);
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
        for (char *chunk = c->chunks; c->len; --c->len) {
            c->destroy(chunk);
            chunk += c->width;
        }
    }
    void *resizeEventArgs = c->resizeEventArgs;
    free_(c->chunks);
    free_(c);
    return resizeEventArgs;
}

void *axc_destroySoft(axchunk *c) {
    void *chunks = c->chunks;
    free_(c);
    return chunks;
}

bool axc_resize(axchunk *c, uint64_t size) {
    size += !size;
    if (size == c->cap)
        return false;
    intptr_t oldChunks = (intptr_t) c->chunks;
    void *chunks = realloc_(c->chunks, size * c->width);
    if (!chunks)
        return true;
    ptrdiff_t offset = (intptr_t) chunks - oldChunks;
    c->chunks = chunks;
    c->cap = size;
    if (c->resizeEventHandler)
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
    char *chunk = c->chunks;
    for (uint64_t i = 0; i < c->len; ++i) {
        if (!f(chunk, arg))
            return c;
        chunk += c->width;
    }
    return c;
}

axchunk *axc_filter(axchunk *c, bool (*f)(const void *, void *), void *arg) {
    const bool shouldDestroy = c->destroy;
    char *chunk = c->chunks;
    char *filterChunk = c->chunks;
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
        for (char *chunk = c->chunks; c->len; --c->len) {
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

void *axc_internalCopy(axchunk *c) {
    uint64_t size = MAX(c->len * c->width, 1);
    void *copy = malloc_(size);
    if (!copy)
        return NULL;
    return memcpy(copy, c->chunks, size);
}

axchunk *axc_copy(axchunk *c) {
    axchunk *copy = axc_newSized(c->width, c->cap);
    if (!copy)
        return NULL;
    memcpy(copy->chunks, c->chunks, c->width * c->len);
    copy->len = c->len;
    return copy;
}

bool axc_write(axchunk *c, uint64_t i, void *chunks, uint64_t chkcount) {
    if (i + chkcount > c->cap) {
        uint64_t size1 = (c->cap << 1) | 1;
        uint64_t size2 = i + chkcount;
        if (axc_resize(c, MAX(size1, size2)))
            return true;
    }
    if (c->destroy) {
        char *chunk = axc__index__(c, i);
        for (uint64_t k = 0; k < chkcount && k + i < c->len; ++k) {
            c->destroy(chunk);
            chunk += c->width;
        }
    }
    axc__quick_memmove__(axc__index__(c, i), chunks, chkcount * c->width);
    c->len = MAX(i + chkcount, c->len);
    return false;
}


uint64_t axc_read(axchunk *c, uint64_t i, void *chunks, uint64_t chkcount) {
    if (i >= c->len)
        return 0;
    if (i + chkcount > c->len)
        chkcount -= i + chkcount - c->len;
    axc__quick_memmove__(chunks, axc__index__(c, i), chkcount * c->width);
    return chkcount;
}
