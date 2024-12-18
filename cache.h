#ifndef CACHE_H
#define CACHE_H

#include <time.h>

#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 256

typedef struct CacheItem {
    char key[MAX_KEY_LENGTH];          // Key of the cache item
    char value[MAX_VALUE_LENGTH];      // Value of the cache item
    time_t expiry;                     // Expiry time (0 if no expiry)
    struct CacheItem *next;            // Pointer to the next item (for LRU)
    struct CacheItem *prev;            // Pointer to the previous item (for LRU)
} CacheItem;

typedef struct Cache {
    CacheItem *head;   // Most recently used item
    CacheItem *tail;   // Least recently used item
    int size;          // Current size of the cache
} Cache;
Cache *create_cache();
void cache_set(Cache *cache, const char *key, const char *value, int ttl);
char *cache_get(Cache *cache, const char *key);
void cache_delete(Cache *cache, const char *key);
void free_cache(Cache *cache);

#endif // CACHE_H
