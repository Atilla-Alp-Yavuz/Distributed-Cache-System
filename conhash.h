#ifndef HASH_RING_H
#define HASH_RING_H

#include <stdint.h>

#define MAX_NODES 10

typedef struct Node {
    char address[256];
    uint32_t hash;
} Node;

typedef struct HashRing {
    Node nodes[MAX_NODES];
    int node_count;
} HashRing;

extern HashRing ring;


uint32_t hash(const char *key);


void add_node(HashRing *ring, const char *address);


const char *get_node(HashRing *ring, const char *key);

void remove_node(HashRing *ring, const char *address);


const char *get_secondary_node(HashRing *ring, const char *key);


#endif