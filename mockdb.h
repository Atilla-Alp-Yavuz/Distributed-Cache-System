#ifndef MOCKDB_H
#define MOCKDB_H

typedef struct MockDB {
    char keys[100][150];
    char values[100][150];
    int count;
} MockDB;

MockDB *create_mockdb();
void db_set(MockDB *db, const char *key, const char *value);
char *db_get(MockDB *db, const char *key);
void db_delete(MockDB *db, const char *key);
void free_mockdb(MockDB *db);

#endif
