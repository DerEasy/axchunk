//
// Created by easy on 16.06.24.
//

#ifndef AXCHUNK_AXCHUNK_H
#define AXCHUNK_AXCHUNK_H

#define axc_index(c, i) (((char *) (c)->items + (i) * (c)->width))

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct axchunk {
    void *items;
    uint64_t len;
    uint64_t cap;
    uint64_t width;
} axchunk;

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
 * Destroy an axchunk.
 */
static inline void axc_destroy(axchunk *c) {
    free(c->items);
    free(c);
}

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
 * Push an item to the end of an axchunk. This operation may resize the axchunk.
 * @param item Pointer to item of chunk-width size to copy into axchunk.
 * @return True iff OOM, in which case nothing is done.
 */
static inline bool axc_push(axchunk *c, void *item) {
    if (c->len >= c->cap && axc_resize(c, (c->cap << 1) | 1))
        return true;
    memcpy(axc_index(c, c->len++), item, c->width);
    return false;
}

/**
 * Pop the last item off the end of an axchunk. That item is subsequently removed. Nothing is done in the case there
 * are no chunks currently occupied.
 * @param dest Pointer to buffer where the item will be written into.
 * @return The destination pointer.
 */
static inline void *axc_pop(axchunk *c, void *dest) {
    if (c->len)
        memmove(dest, axc_index(c, --c->len), c->width);
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
        memmove(dest, axc_index(c, i), c->width);
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
    memmove(axc_index(c, i), item, c->width);
    return false;
}

/**
 * Swap the contents of two chunks. Does nothing if any index is out of range.
 * @param i1 Index of one chunk.
 * @param i2 Index of another chunk.
 * @return Self.
 */
axchunk *axc_swap(axchunk *c, uint64_t i1, uint64_t i2);

#undef axc_index
#endif //AXCHUNK_AXCHUNK_H
