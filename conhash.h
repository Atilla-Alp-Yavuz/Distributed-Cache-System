#ifndef HASH_RING_H
#define HASH_RING_H

#include <stdint.h>  // For uint32_t
#include <openssl/evp.h>  // For EVP interface

// Maximum number of nodes in the hash ring
#define MAX_NODES 10

// Node structure representing a server in the hash ring
typedef struct Node {
    char address[256];  // Server address (e.g., "127.0.0.1:8080")
    uint32_t hash;      // Hash of the server's address
} Node;

// HashRing structure representing the consistent hash ring
typedef struct HashRing {
    Node nodes[MAX_NODES];  // Array of nodes
    int node_count;         // Number of nodes currently in the ring
} HashRing;

/**
 * Hash a string using EVP for MD5.
 * @param key The key to be hashed.
 * @return A 32-bit hash value of the key.
 */
uint32_t hash(const char *key);

/**
 * Add a node to the hash ring.
 * @param ring Pointer to the HashRing.
 * @param address The address of the node to add (e.g., "127.0.0.1:8080").
 */
void add_node(HashRing *ring, const char *address);

/**
 * Find the appropriate node for a given key.
 * @param ring Pointer to the HashRing.
 * @param key The key to map to a node.
 * @return The address of the node responsible for the key.
 */
const char *get_node(HashRing *ring, const char *key);

#endif // HASH_RING_H
