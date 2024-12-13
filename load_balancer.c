#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "conhash.h"

#define BUFFER_SIZE 1024
#define LB_PORT 9090       // Port for the load balancer
#define ANNOUNCE_PORT 9091 // Port for servers to announce themselves
#define HEALTH_CHECK_INTERVAL 5 // Health check interval in seconds

HashRing ring = {0}; // Global consistent hash ring
pthread_mutex_t lock;

// Function to check server health
int is_server_alive(const char *server_address) {
    char ip[256];
    int port;
    sscanf(server_address, "%[^:]:%d", ip, &port);

    int sock;
    struct sockaddr_in server_addr;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return 0; // Server not reachable
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Try to connect to the server
    int result = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    close(sock);

    return result == 0; // Server is alive if connection succeeds
}

// Health check thread function
void *health_check(void *arg) {
    while (1) {
        sleep(HEALTH_CHECK_INTERVAL); // Wait for the next health check

        pthread_mutex_lock(&lock);

        // Iterate through the hash ring and check server health
        for (int i = 0; i < ring.node_count; i++) {
            const char *server_address = ring.nodes[i].address;
            if (!is_server_alive(server_address)) {
                printf("Server '%s' is down. Removing from the ring.\n", server_address);
                remove_node(&ring, server_address);
            }
        }

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

// Forward request to the appropriate server
void forward_to_server(const char *server_address, const char *client_request, int client_socket) {
    char ip[256];
    int port;
    sscanf(server_address, "%[^:]:%d", ip, &port);

    int server_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket for connecting to server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create server socket");
        send(client_socket, "Error: Unable to connect to server\n", 36, 0);
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Connect to the server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        send(client_socket, "Error: Server connection failed\n", 34, 0);
        close(server_socket);
        return;
    }

    // Forward client request to server
    send(server_socket, client_request, strlen(client_request), 0);

    // Receive server response
    int bytes_received = recv(server_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        // Append server address to response
        char enhanced_response[BUFFER_SIZE];
        snprintf(enhanced_response, BUFFER_SIZE, "Server: %s | Response: %s", server_address, buffer);
        send(client_socket, enhanced_response, strlen(enhanced_response), 0);
    }

    close(server_socket);
}

// Handle client connections
void *handle_client(void *client_socket_ptr) {
    int client_socket = *(int *)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[BUFFER_SIZE] = {0};
    recv(client_socket, buffer, BUFFER_SIZE, 0);

    // Parse the key from the client request
    char command[10], key[256];
    sscanf(buffer, "%s %s", command, key);

    // Get the appropriate server for the key from the ring
    pthread_mutex_lock(&lock);
    const char *server_address = get_node(&ring, key);
    pthread_mutex_unlock(&lock);

    if (server_address) {
        printf("Forwarding request for key '%s' to server '%s'\n", key, server_address);
        forward_to_server(server_address, buffer, client_socket);
    } else {
        send(client_socket, "Error: No available server\n", 28, 0);
    }

    close(client_socket);
    return NULL;
}

// Handle server announcements
void *handle_server_announcement(void *arg) {
    int announce_socket;
    struct sockaddr_in announce_addr, server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket for server announcements
    announce_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (announce_socket < 0) {
        perror("Failed to create announcement socket");
        exit(EXIT_FAILURE);
    }

    announce_addr.sin_family = AF_INET;
    announce_addr.sin_port = htons(ANNOUNCE_PORT);
    announce_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(announce_socket, (struct sockaddr *)&announce_addr, sizeof(announce_addr)) < 0) {
        perror("Failed to bind announcement socket");
        exit(EXIT_FAILURE);
    }

    printf("Listening for server announcements on port %d...\n", ANNOUNCE_PORT);

    while (1) {
        int bytes_received = recvfrom(announce_socket, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&server_addr, &addr_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            pthread_mutex_lock(&lock);
            add_node(&ring, buffer); // Add the server to the hash ring
            pthread_mutex_unlock(&lock);
            printf("Server '%s' added to the ring\n", buffer);
        }
    }

    close(announce_socket);
    return NULL;
}

int main() {
    pthread_mutex_init(&lock, NULL);

    // Create threads for server announcements and health checks
    pthread_t announce_thread, health_check_thread;
    pthread_create(&announce_thread, NULL, handle_server_announcement, NULL);
    pthread_create(&health_check_thread, NULL, health_check, NULL);

    int lb_socket, client_socket;
    struct sockaddr_in lb_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket for load balancer
    lb_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (lb_socket < 0) {
        perror("Failed to create load balancer socket");
        return EXIT_FAILURE;
    }

    lb_addr.sin_family = AF_INET;
    lb_addr.sin_port = htons(LB_PORT);
    lb_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(lb_socket, (struct sockaddr *)&lb_addr, sizeof(lb_addr)) < 0) {
        perror("Failed to bind load balancer socket");
        return EXIT_FAILURE;
    }

    if (listen(lb_socket, 10) < 0) {
        perror("Failed to listen on load balancer socket");
        return EXIT_FAILURE;
    }

    printf("Load balancer is running on port %d...\n", LB_PORT);

    while (1) {
        client_socket = accept(lb_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Failed to accept client connection");
            continue;
        }

        pthread_t thread;
        int *client_socket_ptr = malloc(sizeof(int));
        *client_socket_ptr = client_socket;
        pthread_create(&thread, NULL, handle_client, client_socket_ptr);
        pthread_detach(thread);
    }

    close(lb_socket);
    pthread_mutex_destroy(&lock);

    return 0;
}
