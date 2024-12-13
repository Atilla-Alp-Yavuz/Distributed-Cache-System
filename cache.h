#ifndef CACHE_H
#define CACHE_H

#include <time.h>

#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 256

// Cache item structure
typedef struct CacheItem {
    char key[MAX_KEY_LENGTH];          // Key of the cache item
    char value[MAX_VALUE_LENGTH];      // Value of the cache item
    time_t expiry;                     // Expiry time (0 if no expiry)
    struct CacheItem *next;            // Pointer to the next item (for LRU)
    struct CacheItem *prev;            // Pointer to the previous item (for LRU)
} CacheItem;

// Cache structure
typedef struct Cache {
    CacheItem *head;   // Most recently used item
    CacheItem *tail;   // Least recently used item
    int size;          // Current size of the cache
} Cache;

/**
 * Create a new cache.
 * @return Pointer to the newly created cache.
 */
Cache *create_cache();

/**
 * Set a key-value pair in the cache.
 * @param cache Pointer to the cache.
 * @param key The key to set.
 * @param value The value to set.
 * @param ttl Time-to-live in seconds (0 for no expiry).
 */
void cache_set(Cache *cache, const char *key, const char *value, int ttl);

/**
 * Get the value associated with a key from the cache.
 * @param cache Pointer to the cache.
 * @param key The key to retrieve.
 * @return Pointer to the value if found and not expired; NULL otherwise.
 */
char *cache_get(Cache *cache, const char *key);

/**
 * Delete a key from the cache.
 * @param cache Pointer to the cache.
 * @param key The key to delete.
 */
void cache_delete(Cache *cache, const char *key);

/**
 * Free all memory associated with the cache.
 * @param cache Pointer to the cache.
 */
void free_cache(Cache *cache);

#endif // CACHE_H
