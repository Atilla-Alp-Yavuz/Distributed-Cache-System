#ifndef CACHE_H
#define CACHE_H


#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 256

typedef struct CacheItem {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    time_t expiry;
    struct CacheItem *next;
    struct CacheItem *prev;
} CacheItem;

typedef struct Cache {
    CacheItem *head;
    CacheItem *tail;
    int size;
} Cache;
Cache *create_cache();
void cache_set(Cache *cache, const char *key, const char *value, int ttl);
char *cache_get(Cache *cache, const char *key);
void cache_delete(Cache *cache, const char *key);
void free_cache(Cache *cache);

#endif
