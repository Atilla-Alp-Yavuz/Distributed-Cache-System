#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "cache.h"

#define BUFFER_SIZE 1024

void handle_client(int client_socket, Cache *cache) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        // Receive data from the client
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            break; // Connection closed
        }

        // Parse the command
        char command[10], key[MAX_KEY_LENGTH], value[MAX_VALUE_LENGTH];
        int ttl;
        sscanf(buffer, "%s %s %s %d", command, key, value, &ttl);

        char response[BUFFER_SIZE] = {0};
        if (strcmp(command, "set") == 0) {
            cache_set(cache, key, value, ttl);
            snprintf(response, BUFFER_SIZE, "OK");
        } else if (strcmp(command, "get") == 0) {
            char *result = cache_get(cache, key);
            snprintf(response, BUFFER_SIZE, "%s", result ? result : "null");
        } else if (strcmp(command, "delete") == 0) {
            cache_delete(cache, key);
            snprintf(response, BUFFER_SIZE, "OK");
        } else {
            snprintf(response, BUFFER_SIZE, "Invalid command");
        }

        // Send response to the client
        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    Cache *cache = create_cache();

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    // Bind socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return EXIT_FAILURE;
    }

    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        return EXIT_FAILURE;
    }
    printf("Server is listening on port %d\n", port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(client_socket, cache);
    }

    free_cache(cache);
    close(server_socket);
    return 0;
}
