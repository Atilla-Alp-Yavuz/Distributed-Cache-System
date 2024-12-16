#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "cache.h"
#include "mockdb.h"

#define BUFFER_SIZE 1024

void announce_to_load_balancer(const char *server_address) {
    int sock;
    struct sockaddr_in lb_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Failed to create socket for announcement");
        return;
    }

    lb_addr.sin_family = AF_INET;
    lb_addr.sin_port = htons(9091);
    inet_pton(AF_INET, "127.0.0.1", &lb_addr.sin_addr);

    sendto(sock, server_address, strlen(server_address), 0, (struct sockaddr *)&lb_addr, sizeof(lb_addr));
    printf("Announced server '%s' to the load balancer\n", server_address);
    close(sock);
}

void handle_client(int client_socket, Cache *cache, MockDB *db) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) break;

        char command[10], key[MAX_KEY_LENGTH], value[MAX_VALUE_LENGTH];
        int ttl;
        sscanf(buffer, "%s %s %s %d", command, key, value, &ttl);

        char response[BUFFER_SIZE] = {0};

        if (strcmp(command, "set") == 0) {
            cache_set(cache, key, value, ttl);
            db_set(db, key, value);
            snprintf(response, BUFFER_SIZE, "OK");
        } else if (strcmp(command, "get") == 0) {
            char *result = cache_get(cache, key);
            if (result) {
                printf("Cache Hit: %s\n", key);
                snprintf(response, BUFFER_SIZE, "%s", result);
            } else {
                printf("Cache Miss: %s\n", key);
                result = db_get(db, key);
                if (result) {
                    cache_set(cache, key, result, 60); // Cache the value for 60 seconds
                    snprintf(response, BUFFER_SIZE, "%s", result);
                } else {
                    snprintf(response, BUFFER_SIZE, "null");
                }
            }
        } else if (strcmp(command, "delete") == 0) {
            cache_delete(cache, key);
            db_delete(db, key);
            snprintf(response, BUFFER_SIZE, "OK");
        } else {
            snprintf(response, BUFFER_SIZE, "Invalid command");
        }

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
    MockDB *db = create_mockdb();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return EXIT_FAILURE;
    }

    char server_address[256];
    snprintf(server_address, sizeof(server_address), "127.0.0.1:%d", port);
    announce_to_load_balancer(server_address);

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
        handle_client(client_socket, cache, db);
    }

    free_cache(cache);
    free_mockdb(db);
    close(server_socket);
    return 0;
}
