/*
 * This file is part of libnica.
 *
 * Copyright (C) 2016 Intel Corporation
 *
 * libnica is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

#define INITIAL_SIZE 61

/* When we're "full", 70% */
#define FULL_FACTOR 0.7

/* Multiple to increase bucket size by */
#define INCREASE_FACTOR 4

/**
 * An bucket/chain within the hashmap
 */
typedef struct NcHashmapEntry {
        void *hash;                  /**<The key for this item */
        void *value;                 /**<Value for this item */
        struct NcHashmapEntry *next; /**<Next item in the chain */
        bool occ;                    /**<Whether this bucket is occupied */
} NcHashmapEntry;

/**
 * A NcHashmap
 */
struct NcHashmap {
        int size;                /**<Current size of the hashmap */
        int next_size;           /**<Next size at which we need to resize */
        int n_buckets;           /**<Current number of buckets */
        NcHashmapEntry *buckets; /**<Stores our bucket chains */

        nc_hash_create_func hash;     /**<Hash generation function */
        nc_hash_compare_func compare; /**<Key comparison function */
        nc_hash_free_func key_free;   /**<Cleanup function for keys */
        nc_hash_free_func value_free; /**<Cleanup function for values */
};

/**
 * Iteration object
 */
typedef struct _NcHashmapIter {
        int bucket;     /**<Current bucket position */
        NcHashmap *map; /**<Associated NcHashmap */
        void *item;     /**<Current item in this iteration */
} _NcHashmapIter;

static void nc_hashmap_update_next_size(NcHashmap *self);
static bool nc_hashmap_resize(NcHashmap *self);

static inline bool nc_hashmap_maybe_resize(NcHashmap *self)
{
        if (!self) {
                return false;
        }
        if (self->size >= self->next_size) {
                return true;
        }
        return false;
}

static NcHashmap *nc_hashmap_new_internal(nc_hash_create_func create, nc_hash_compare_func compare,
                                          nc_hash_free_func key_free, nc_hash_free_func value_free)
{
        NcHashmap *map = NULL;
        NcHashmapEntry *buckets = NULL;

        map = calloc(1, sizeof(NcHashmap));
        if (!map) {
                return NULL;
        }

        buckets = calloc(INITIAL_SIZE, sizeof(NcHashmapEntry));
        if (!buckets) {
                free(map);
                return NULL;
        }
        map->buckets = buckets;
        map->n_buckets = INITIAL_SIZE;
        map->hash = create ? create : nc_simple_hash;
        map->compare = compare ? compare : nc_simple_compare;
        map->key_free = key_free;
        map->value_free = value_free;
        map->size = 0;

        nc_hashmap_update_next_size(map);

        return map;
}

NcHashmap *nc_hashmap_new(nc_hash_create_func create, nc_hash_compare_func compare)
{
        return nc_hashmap_new_internal(create, compare, NULL, NULL);
}

NcHashmap *nc_hashmap_new_full(nc_hash_create_func create, nc_hash_compare_func compare,
                               nc_hash_free_func key_free, nc_hash_free_func value_free)
{
        return nc_hashmap_new_internal(create, compare, key_free, value_free);
}

static inline unsigned nc_hashmap_get_hash(NcHashmap *self, const void *key)
{
        unsigned hash = self->hash(key);
        return hash;
}

static bool nc_hashmap_insert_bucket(NcHashmap *self, NcHashmapEntry *buckets, int n_buckets,
                                     unsigned hash, const void *key, void *value)
{
        NcHashmapEntry *row = &(buckets[hash % n_buckets]);
        NcHashmapEntry *head = NULL;
        NcHashmapEntry *parent = head = row;
        NcHashmapEntry *tomb = NULL;
        int ret = 1;

        while (row) {
                if (!row->occ) {
                        tomb = row;
                }
                parent = row;
                if (row->occ && self->compare(row->hash, key)) {
                        if (self->value_free) {
                                self->value_free(row->value);
                        }
                        if (self->key_free) {
                                self->key_free(row->hash);
                        }
                        row->hash = (void *)key;
                        row->value = value;
                        return 0;
                }
                row = row->next;
        }

        if (tomb) {
                row = tomb;
        }

        if (!row) {
                row = calloc(1, sizeof(NcHashmapEntry));
                if (!row) {
                        return -1;
                }
        }

        row->hash = (void *)key;
        row->value = value;
        row->occ = true;
        if (parent != row && parent) {
                parent->next = row;
        }

        return ret;
}

bool nc_hashmap_put(NcHashmap *self, const void *key, void *value)
{
        if (!self) {
                return false;
        }
        int inc;

        if (nc_hashmap_maybe_resize(self)) {
                if (!nc_hashmap_resize(self)) {
                        return false;
                }
        }
        unsigned hash = nc_hashmap_get_hash(self, key);
        inc = nc_hashmap_insert_bucket(self, self->buckets, self->n_buckets, hash, key, value);
        if (inc >= 0) {
                self->size += inc;
                return true;
        } else {
                return false;
        }
}

static NcHashmapEntry *nc_hashmap_get_entry(NcHashmap *self, const void *key)
{
        if (!self) {
                return NULL;
        }

        unsigned hash = nc_hashmap_get_hash(self, key);
        NcHashmapEntry *row = &(self->buckets[hash % self->n_buckets]);

        while (row) {
                if (self->compare(row->hash, key)) {
                        return row;
                }
                row = row->next;
        }
        return NULL;
}

void *nc_hashmap_get(NcHashmap *self, const void *key)
{
        if (!self) {
                return NULL;
        }

        NcHashmapEntry *row = nc_hashmap_get_entry(self, key);
        if (row) {
                return row->value;
        }
        return NULL;
}

static bool nc_hashmap_remove_internal(NcHashmap *self, const void *key, bool remove)
{
        if (!self) {
                return false;
        }
        NcHashmapEntry *row = nc_hashmap_get_entry(self, key);

        if (!row) {
                return false;
        }

        if (remove) {
                if (self->key_free) {
                        self->key_free(row->hash);
                }
                if (self->value_free) {
                        self->value_free(row->value);
                }
        }
        self->size -= 1;
        row->hash = NULL;
        row->value = NULL;
        row->occ = false;

        return true;
}

bool nc_hashmap_steal(NcHashmap *self, const void *key)
{
        return nc_hashmap_remove_internal(self, key, false);
}

bool nc_hashmap_remove(NcHashmap *self, const void *key)
{
        return nc_hashmap_remove_internal(self, key, true);
}

bool nc_hashmap_contains(NcHashmap *self, const void *key)
{
        return (nc_hashmap_get(self, key)) != NULL;
}

static inline void nc_hashmap_free_bucket(NcHashmap *self, NcHashmapEntry *bucket, bool nuke)
{
        if (!self) {
                return;
        }
        NcHashmapEntry *tmp = bucket;
        NcHashmapEntry *bk = bucket;
        NcHashmapEntry *root = bucket;

        while (tmp) {
                bk = NULL;
                if (tmp->next) {
                        bk = tmp->next;
                }

                if (nuke && tmp->occ) {
                        if (self->key_free) {
                                self->key_free(tmp->hash);
                        }
                        if (self->value_free) {
                                self->value_free(tmp->value);
                        }
                }
                if (tmp != root) {
                        free(tmp);
                }
                tmp = bk;
        }
}

void nc_hashmap_free(NcHashmap *self)
{
        if (!self) {
                return;
        }
        for (int i = 0; i < self->n_buckets; i++) {
                NcHashmapEntry *row = &(self->buckets[i]);
                nc_hashmap_free_bucket(self, row, true);
        }
        if (self->buckets) {
                free(self->buckets);
        }

        free(self);
}

static void nc_hashmap_update_next_size(NcHashmap *self)
{
        if (!self) {
                return;
        }
        self->next_size = (int)(self->n_buckets * FULL_FACTOR);
}

int nc_hashmap_size(NcHashmap *self)
{
        if (!self) {
                return -1;
        }
        return self->size;
}

static bool nc_hashmap_resize(NcHashmap *self)
{
        if (!self || !self->buckets) {
                return false;
        }

        NcHashmapEntry *old_buckets = self->buckets;
        NcHashmapEntry *new_buckets = NULL;
        NcHashmapEntry *entry = NULL;
        int incr;

        int old_size, new_size;
        int items = 0;

        new_size = old_size = self->n_buckets;
        new_size *= INCREASE_FACTOR;

        new_buckets = calloc(new_size, sizeof(NcHashmapEntry));
        if (!new_buckets) {
                return false;
        }

        for (int i = 0; i < old_size; i++) {
                entry = &(old_buckets[i]);
                while (entry) {
                        if (entry->occ) {
                                unsigned hash = nc_hashmap_get_hash(self, entry->hash);
                                if ((incr = nc_hashmap_insert_bucket(self,
                                                                     new_buckets,
                                                                     new_size,
                                                                     hash,
                                                                     entry->hash,
                                                                     entry->value)) > 0) {
                                        items += incr;
                                } else {
                                        /* Likely a memory issue */
                                        goto failure;
                                }
                        }
                        entry = entry->next;
                }
        }
        /* Successfully resized - do this separately because we need to
         * gaurantee old data is preserved */
        for (int i = 0; i < old_size; i++) {
                nc_hashmap_free_bucket(self, &(old_buckets[i]), false);
        }

        free(old_buckets);
        self->n_buckets = new_size;
        self->size = items;
        self->buckets = new_buckets;

        nc_hashmap_update_next_size(self);
        return true;

failure:
        for (int i = 0; i < new_size; i++) {
                nc_hashmap_free_bucket(self, &(new_buckets[i]), true);
        }
        free(new_buckets);
        return false;
}

void nc_hashmap_iter_init(NcHashmap *map, NcHashmapIter *citer)
{
        _NcHashmapIter *iter = NULL;
        if (!map || !citer) {
                return;
        }
        iter = (_NcHashmapIter *)citer;
        _NcHashmapIter it = {
                .bucket = -1, .map = map, .item = NULL,
        };
        *iter = it;
}

bool nc_hashmap_iter_next(NcHashmapIter *citer, void **key, void **value)
{
        _NcHashmapIter *iter = NULL;
        NcHashmapEntry *item = NULL;
        NcHashmap *map = NULL;
        int n_buckets = 0;

        if (!citer) {
                return false;
        }

        iter = (_NcHashmapIter *)citer;
        map = iter->map;
        if (!map) {
                return false;
        }
        n_buckets = map->n_buckets;
        item = iter->item;

        for (;;) {
                if (iter->bucket >= n_buckets) {
                        if (item && !item->next) {
                                return false;
                        }
                }
                if (!item) {
                        iter->bucket++;
                        if (iter->bucket > n_buckets - 1) {
                                return false;
                        }
                        item = &(map->buckets[iter->bucket]);
                }
                if (item && item->occ) {
                        goto success;
                }
                item = item->next;
        }
        return false;

success:
        iter->item = item->next;
        if (key) {
                *key = item->hash;
        }
        if (value) {
                *value = item->value;
        }

        return true;
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
