#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "mockdb.h"

// Create a mock database and populate it with dummy data
MockDB *create_mockdb() {
    MockDB *db = (MockDB *)malloc(sizeof(MockDB));
    db->count = 0;

    // Add some dummy data
    for (int i = 0; i < 10; i++) {
        snprintf(db->keys[i], MAX_KEY_LENGTH, "key%d", i);
        snprintf(db->values[i], MAX_VALUE_LENGTH, "value%d", i);
        db->count++;
    }
    return db;
}

// Set a key-value pair in the mock database
void db_set(MockDB *db, const char *key, const char *value) {
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->keys[i], key) == 0) {
            strncpy(db->values[i], value, MAX_VALUE_LENGTH);
            return;
        }
    }
    strncpy(db->keys[db->count], key, MAX_KEY_LENGTH);
    strncpy(db->values[db->count], value, MAX_VALUE_LENGTH);
    db->count++;
}

// Get a value from the mock database
char *db_get(MockDB *db, const char *key) {
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->keys[i], key) == 0) {
            return db->values[i];
        }
    }
    return NULL; // Not found
}

// Delete a key from the mock database
void db_delete(MockDB *db, const char *key) {
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->keys[i], key) == 0) {
            // Shift remaining entries left
            for (int j = i; j < db->count - 1; j++) {
                strncpy(db->keys[j], db->keys[j + 1], MAX_KEY_LENGTH);
                strncpy(db->values[j], db->values[j + 1], MAX_VALUE_LENGTH);
            }
            db->count--;
            return;
        }
    }
}

// Free mock database memory
void free_mockdb(MockDB *db) {
    free(db);
}
