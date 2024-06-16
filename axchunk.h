//
// Created by easy on 16.06.24.
//

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef AXCHUNK_AXCHUNK_H
#define AXCHUNK_AXCHUNK_H

/**
 * Same as axc_index, but without bounds checking.
 */
#define axc__index__(c, i) (((char *) (c)->items + (i) * (c)->width))

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * axchunk is chunk vector library with some functional programming concepts and utility functions included.
 *
 * Each chunk has a constant size ("width"). axchunk is useful if you want to avoid excessive amounts of memory
 * allocations and memory wastage, because unlike a regular vector, which stores pointers to its objects, axchunk
 * stores every object directly in its internal array.
 *
 * A destructor function may be supplied. There is no default destructor. The destructor is called on any chunk which
 * is irrevocably removed from the axchunk.
 *
 * The struct definition of axchunk is given in its header for optimisation purposes only. To use axchunk, you must
 * rely solely on the functions of the library.
 */
typedef struct axchunk {
    void *items;
    uint64_t len;
    uint64_t cap;
    uint64_t width;
    void (*destroy)(void *);
} axchunk;

/**
 * This is an internal function of the axchunk library.
 */
static inline void *axc__quick_memcpy__(void *dst, void *src, size_t n) {
    switch (n) {
    case 1: return memcpy(dst, src, 1);
    case 2: return memcpy(dst, src, 2);
    case 4: return memcpy(dst, src, 4);
    case 8: return memcpy(dst, src, 8);
    case 12: return memcpy(dst, src, 12);
    case 16: return memcpy(dst, src, 16);
    default: return memcpy(dst, src, n);
    }
}

/**
 * This is an internal function of the axchunk library.
 */
static inline void *axc__quick_memmove__(void *dst, void *src, size_t n) {
    switch (n) {
    case 1: return memmove(dst, src, 1);
    case 2: return memmove(dst, src, 2);
    case 4: return memmove(dst, src, 4);
    case 8: return memmove(dst, src, 8);
    case 16: return memmove(dst, src, 16);
    default: return memmove(dst, src, n);
    }
}

/**
 * Creates a new axchunk with default capacity.
 * @param width Size of individual chunks.
 * @return New axchunk or NULL iff OOM.
 */
axchunk *axc_new(uint64_t width);

/**
 * Creates a new axchunk with given capacity.
 * @param width Size of individual chunks.
 * @param size Number of chunks to allocate.
 * @return New axchunk or NULL iff OOM.
 */
axchunk *axc_newSized(uint64_t width, uint64_t size);

/**
 * If a destructor is available, call it on each chunk. In any case, the axchunk is destroyed.
 */
void axc_destroy(axchunk *c);

/**
 * Sets a new capacity for the axchunk.
 * @param size Number of chunks this axchunk should be able to hold at maximum.
 * @return True iff OOM.
 */
bool axc_resize(axchunk *c, uint64_t size);

/**
 * Unsigned number of occupied chunks.
 * @return Unsigned length of axchunk.
 */
static inline uint64_t axc_ulen(axchunk *c) {
    return c->len;
}

/**
 * Signed number of occupied chunks.
 * @return Signed length of axchunk.
 */
static inline int64_t axc_len(axchunk *c) {
    return (int64_t) c->len;
}

/**
 * Unsigned maximum number of chunks that can be held without resizing.
 * @return Unsigned capacity of axchunk.
 */
static inline uint64_t axc_ucap(axchunk *c) {
    return c->cap;
}

/**
 * Signed maximum number of chunks that can be held without resizing.
 * @return Signed capacity of axchunk.
 */
static inline int64_t axc_cap(axchunk *c) {
    return (int64_t) c->cap;
}

/**
 * Size of each individual chunk in this axchunk.
 * @return Chunk width of an axchunk.
 */
static inline uint64_t axc_width(axchunk *c) {
    return c->width;
}

/**
 * Pointer to first chunk in this axchunk. Useful for raw access to the internal array.
 * @return Internal array of this axchunk.
 */
static inline void *axc_data(axchunk *c) {
    return c->items;
}

/**
 * Get a pointer to the chunk at some index.
 * @param i Index of chunk.
 * @return Pointer to chunk or NULL if index out of range.
 */
static inline void *axc_index(axchunk *c, uint64_t i) {
    return i < c->len ? (char *) c->items + i * c->width : NULL;
}

/**
 * Set destructor function. Type must match void (*)(void *). The destructor is passed a pointer to a chunk, but beware
 * the pointed-to memory chunk has not itself been allocated separately.
 * @param destroy Destructor or NULL to deactivate destructor features.
 * @return Self.
 */
static inline axchunk *axc_setDestructor(axchunk *c, void (*destroy)(void *)) {
    c->destroy = destroy;
    return c;
}

/**
 * Get destructor function. Type is void (*)(void *).
 * @return Destructor or NULL if not set.
 */
static inline void (*axc_getDestructor(axchunk *c))(void *) {
    return c->destroy;
}

/**
 * Push an item to the end of an axchunk. This operation may resize the axchunk.
 * @param item Pointer to item of chunk-width size to copy into axchunk.
 * @return True iff OOM, in which case nothing is done.
 */
static inline bool axc_push(axchunk *c, void *item) {
    if (c->len >= c->cap && axc_resize(c, (c->cap << 1) | 1))
        return true;
    axc__quick_memcpy__(axc__index__(c, c->len++), item, c->width);
    return false;
}

/**
 * Pop the last item off the end of an axchunk. That item is subsequently removed. Nothing is done in the case there
 * currently are no chunks occupied.
 * @param dest Pointer to buffer where the item will be written into.
 * @return The destination pointer.
 */
static inline void *axc_pop(axchunk *c, void *dest) {
    if (c->len)
        axc__quick_memmove__(dest, axc__index__(c, --c->len), c->width);
    return dest;
}

/**
 * Get the last item off the end of an axchunk without removing it. Nothing is done in the case there currently are no
 * chunks occupied.
 * @param dest Pointer to buffer where the item will be written into.
 * @return The destination pointer.
 */
static inline void *axc_top(axchunk *c, void *dest) {
    if (c->len)
        axc__quick_memmove__(dest, axc__index__(c, c->len - 1), c->width);
    return dest;
}

/**
 * Get the i-th item of an axchunk and copy it into the memory buffer dest.
 * Does nothing when index is out of range.
 * @param i Index of item to copy.
 * @param dest Pointer to buffer where the item will be written into.
 * @return The destination pointer.
 */
static inline void *axc_get(axchunk *c, uint64_t i, void *dest) {
    if (i < c->len)
        axc__quick_memmove__(dest, axc__index__(c, i), c->width);
    return dest;
}

/**
 * Set the i-th item of an axchunk. If i is equal to the length of the vector, the item is simply pushed into the
 * axchunk, which may regrow the axchunk. If i points to an index of an occupied chunk, that chunk is overwritten
 * with the given item. Otherwise the function fails and does nothing.
 * @param i Index of chunk to overwrite.
 * @param item Item to copy into a chunk.
 * @return True if index out of range or OOM.
 */
static inline bool axc_set(axchunk *c, uint64_t i, void *item) {
    if (i > c->len)
        return true;
    if (i == c->len)
        return axc_push(c, item);
    axc__quick_memmove__(axc__index__(c, i), item, c->width);
    return false;
}

/**
 * Swap the contents of two chunks. Does nothing if any index is out of range.
 * @param i1 Index of one chunk.
 * @param i2 Index of another chunk.
 * @return Self.
 */
axchunk *axc_swap(axchunk *c, uint64_t i1, uint64_t i2);

/**
 * Let f be a function taking (pointer to chunk, optional argument).
 * Call f(x, arg) on each chunk x until f returns false or all chunks of the axchunk have been exhausted.
 * Chunks are iterated from first to last.
 * @param f Function to call on all chunks.
 * @param arg An optional argument passed to the function.
 * @return Self.
 */
axchunk *axc_foreach(axchunk *c, bool (*f)(void *, void *), void *arg);

/**
 * Let f be a predicate taking (pointer to chunk, optional argument).
 * Keep all chunks x in the axchunk that satisfy f(x, arg), remove all those that don't, and close the resulting
 * gaps by contracting the space between all remaining chunks, thus preserving the relative order of the remaining
 * chunks. If a destructor is set, it is called upon all removed chunks. The filter is applied linearly from first
 * to last chunk. O(n).
 * @param f Some predicate to filter the axchunk.
 * @param arg An optional argument passed to the filter.
 * @return Self.
 */
axchunk *axc_filter(axchunk *c, bool (*f)(const void *, void *), void *arg);

/**
 * Remove every chunk in this axchunk and set its length to zero. If a destructor is set, it is called upon
 * each chunk.
 * @return Self.
 */
axchunk *axc_clear(axchunk *c);

/**
 * Remove the last n chunks from this axchunk and call the destructor on them if it is available.
 * @param n Number of chunks to discard. This is automatically clamped to the number of chunks occupied.
 * @return Self.
 */
axchunk *axc_discard(axchunk *c, uint64_t n);

#undef axc__index__
#endif //AXCHUNK_AXCHUNK_H
