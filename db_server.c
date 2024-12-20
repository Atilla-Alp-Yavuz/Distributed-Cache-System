#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "mockdb.h"

#define BUFFER_SIZE 1024
#define MAX_KEY_LENGTH 200
#define MAX_VALUE_LENGTH 200
#define DB_PORT 9092

void handle_client(int client_socket, MockDB *db) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) break;

        char command[10], key[MAX_KEY_LENGTH], value[MAX_VALUE_LENGTH];
        sscanf(buffer, "%s %s %s", command, key, value);

        char response[BUFFER_SIZE] = {0};

        if (strcmp(command, "set") == 0) {
            db_set(db, key, value);
            snprintf(response, BUFFER_SIZE, "OK");
        } else if (strcmp(command, "get") == 0) {
            char *result = db_get(db, key);
            snprintf(response, BUFFER_SIZE, "%s", result ? result : "null");
        } else if (strcmp(command, "delete") == 0) {
            db_delete(db, key);
            snprintf(response, BUFFER_SIZE, "OK");
        } else {
            snprintf(response, BUFFER_SIZE, "Invalid command");
        }

        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    MockDB *db = create_mockdb();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DB_PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return EXIT_FAILURE;
    }

    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        return EXIT_FAILURE;
    }

    printf("Central database server listening on port %d\n", DB_PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(client_socket, db);
    }

    free_mockdb(db);
    close(server_socket);
    return 0;
}
