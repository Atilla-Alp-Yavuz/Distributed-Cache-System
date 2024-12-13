#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "conhash.h"

#define BUFFER_SIZE 1024
#define SERVER_CONFIG "servers.txt" // Configuration file with server addresses

void send_to_server(const char *server_address, const char *command) {
    char ip[256];
    int port;
    sscanf(server_address, "%[^:]:%d", ip, &port);

    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert address from text to binary form
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(sock);
        return;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }

    // Send command
    send(sock, command, strlen(command), 0);

    // Receive response
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Server response: %s\n", buffer);
    }

    close(sock);
}

void load_servers(HashRing *ring) {
    FILE *file = fopen(SERVER_CONFIG, "r");
    if (!file) {
        perror("Failed to open server configuration file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // Remove newline
        add_node(ring, line);
    }

    fclose(file);
}

int main() {
    HashRing ring = {0};

    // Load servers from configuration file
    load_servers(&ring);

    char command[BUFFER_SIZE];

    while (1) {
        printf("Enter command (e.g., set key value ttl, get key, delete key): ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = '\0'; // Remove newline character

        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Parse the key from the command
        char key[256];
        if (sscanf(command, "%*s %s", key) != 1) {
            printf("Invalid command. A key is required.\n");
            continue;
        }

        // Get the appropriate server for the key
        const char *server_address = get_node(&ring, key);
        if (server_address) {
            printf("Key '%s' is mapped to node '%s'\n", key, server_address);
            send_to_server(server_address, command);
        } else {
            printf("No server found for the key\n");
        }
    }

    return 0;
}
