#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cache.h"

#define MAX_CACHE_SIZE 3

Cache *create_cache() {
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    cache->head = cache->tail = NULL;
    cache->size = 0;
    return cache;
}

void move_to_head(Cache *cache, CacheItem *item) {
    if (cache->head == item) {
        return;
    }

    // Remove from current position
    if (item->prev) item->prev->next = item->next;
    if (item->next) item->next->prev = item->prev;
    if (cache->tail == item) cache->tail = item->prev;

    // Move to head
    item->prev = NULL;
    item->next = cache->head;
    if (cache->head) cache->head->prev = item;
    cache->head = item;

    if (!cache->tail) cache->tail = item;
}

void evict_lru(Cache *cache) {
    if (!cache->tail) return;

    CacheItem *to_remove = cache->tail;
    if (cache->tail->prev) {
        cache->tail = cache->tail->prev;
        cache->tail->next = NULL;
    } else {
        cache->head = cache->tail = NULL;
    }

    free(to_remove);
    cache->size--;
}

void cache_set(Cache *cache, const char *key, const char *value, int ttl) {
    // Check if the key already exists
    CacheItem *current = cache->head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            strncpy(current->value, value, MAX_VALUE_LENGTH);
            current->expiry = ttl > 0 ? time(NULL) + ttl : 0;
            move_to_head(cache, current);
            return;
        }
        current = current->next;
    }

    // Add a new item
    CacheItem *new_item = (CacheItem *)malloc(sizeof(CacheItem));
    strncpy(new_item->key, key, MAX_KEY_LENGTH);
    strncpy(new_item->value, value, MAX_VALUE_LENGTH);
    new_item->expiry = ttl > 0 ? time(NULL) + ttl : 0;
    new_item->next = cache->head;
    new_item->prev = NULL;

    if (cache->head) cache->head->prev = new_item;
    cache->head = new_item;
    if (!cache->tail) cache->tail = new_item;

    cache->size++;

    // Evict the least recently used item if the cache is full
    if (cache->size > MAX_CACHE_SIZE) {
        evict_lru(cache);
    }
}

char *cache_get(Cache *cache, const char *key) {
    CacheItem *current = cache->head;
    time_t now = time(NULL);

    while (current) {
        if (strcmp(current->key, key) == 0) {
            // Check if the item has expired
            if (current->expiry != 0 && current->expiry <= now) {
                cache_delete(cache, key); // Remove expired item
                return NULL;
            }

            // Move the accessed item to the head
            move_to_head(cache, current);
            return current->value;
        }
        current = current->next;
    }

    return NULL; // Key not found
}

void cache_delete(Cache *cache, const char *key) {
    CacheItem *current = cache->head;

    while (current) {
        if (strcmp(current->key, key) == 0) {
            // Unlink the item from the list
            if (current->prev) {
                current->prev->next = current->next;
            } else {
                cache->head = current->next;
            }

            if (current->next) {
                current->next->prev = current->prev;
            } else {
                cache->tail = current->prev;
            }

            // Free the memory and update the size
            free(current);
            cache->size--;
            return;
        }
        current = current->next;
    }
}

void free_cache(Cache *cache) {
    CacheItem *current = cache->head;

    while (current) {
        CacheItem *to_free = current;
        current = current->next;
        free(to_free);
    }

    free(cache);
}
