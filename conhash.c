#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "conhash.h"

uint32_t hash(const char *key) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("EVP_MD_CTX_new");
        exit(EXIT_FAILURE);
    }

    if (EVP_DigestInit_ex(mdctx, EVP_md5(), NULL) != 1) {
        perror("EVP_DigestInit_ex");
        exit(EXIT_FAILURE);
    }

    if (EVP_DigestUpdate(mdctx, key, strlen(key)) != 1) {
        perror("EVP_DigestUpdate");
        exit(EXIT_FAILURE);
    }

    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len) != 1) {
        perror("EVP_DigestFinal_ex");
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX_free(mdctx);

    uint32_t hash_value = 0;
    for (int i = 0; i < 4; i++) {
        hash_value = (hash_value << 8) | digest[i];
    }

    return hash_value;
}

void add_node(HashRing *ring, const char *address) {
    if (ring->node_count >= MAX_NODES) {
        fprintf(stderr, "Max nodes reached\n");
        return;
    }

    Node node;
    strncpy(node.address, address, sizeof(node.address) - 1);
    node.hash = hash(address);

    ring->nodes[ring->node_count++] = node;

    for (int i = 0; i < ring->node_count - 1; i++) {
        for (int j = 0; j < ring->node_count - i - 1; j++) {
            if (ring->nodes[j].hash > ring->nodes[j + 1].hash) {
                Node temp = ring->nodes[j];
                ring->nodes[j] = ring->nodes[j + 1];
                ring->nodes[j + 1] = temp;
            }
        }
    }
}

const char *get_node(HashRing *ring, const char *key) {
    if (ring->node_count == 0) {
        fprintf(stderr, "No nodes in the ring\n");
        return NULL;
    }

    uint32_t key_hash = hash(key);

    for (int i = 0; i < ring->node_count; i++) {
        if (key_hash <= ring->nodes[i].hash) {
            return ring->nodes[i].address;
        }
    }

    return ring->nodes[0].address;
}

void remove_node(HashRing *ring, const char *address) {
    int found = 0;

    for (int i = 0; i < ring->node_count; i++) {
        if (strcmp(ring->nodes[i].address, address) == 0) {
            found = 1;

            for (int j = i; j < ring->node_count - 1; j++) {
                ring->nodes[j] = ring->nodes[j + 1];
            }

            ring->node_count--;
            printf("Node '%s' removed from the hash ring\n", address);
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "Node '%s' not found in the hash ring\n", address);
    }
}

const char *get_secondary_node(HashRing *ring, const char *key) {
    if (ring->node_count < 2) {
        return NULL;
    }

    uint32_t key_hash = hash(key);

    for (int i = 0; i < ring->node_count; i++) {
        if (key_hash <= ring->nodes[i].hash) {
            return ring->nodes[(i + 1) % ring->node_count].address;
        }
    }

    return ring->nodes[1].address;
}

