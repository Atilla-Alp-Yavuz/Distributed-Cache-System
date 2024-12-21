#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "conhash.h"

#define BUFFER_SIZE 1024
#define LB_PORT 9090
#define ANNOUNCE_PORT 9091
#define HEALTH_CHECK_INTERVAL 5

pthread_mutex_t lock;

int is_server_alive(const char *server_address) {
    char ip[256];
    int port;
    sscanf(server_address, "%[^:]:%d", ip, &port);

    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    int result = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    close(sock);

    return result == 0;
}

void *health_check(void *arg) {
    while (1) {
        sleep(HEALTH_CHECK_INTERVAL);
        pthread_mutex_lock(&lock);

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

const char *get_next_node(const char *current_server_address) {
    pthread_mutex_lock(&lock);
    int current_index = -1;
    for (int i = 0; i < ring.node_count; i++) {
        if (strcmp(ring.nodes[i].address, current_server_address) == 0) {
            current_index = i;
            break;
        }
    }

    if (current_index == -1) {
        pthread_mutex_unlock(&lock);
        return NULL;
    }


    int next_index = (current_index + 1) % ring.node_count;
    const char *next_server_address = ring.nodes[next_index].address;

    pthread_mutex_unlock(&lock);
    return next_server_address;
}

void forward_to_server(const char *server_address, const char *client_request, int client_socket) {
    char ip[256];
    int port;
    sscanf(server_address, "%[^:]:%d", ip, &port);

    int server_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create server socket");
        send(client_socket, "Error: Unable to connect to server\n", 36, 0);
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to primary server");
        send(client_socket, "Error: Server connection failed\n", 34, 0);
        close(server_socket);
        return;
    }

    send(server_socket, client_request, strlen(client_request), 0);

    int bytes_received = recv(server_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        char enhanced_response[BUFFER_SIZE];
        snprintf(enhanced_response, BUFFER_SIZE, "Primary Server: %s | Response: %s", server_address, buffer);
        send(client_socket, enhanced_response, strlen(enhanced_response), 0);
    }

    close(server_socket);

    const char *replica_server_address = get_next_node(server_address);
    if (replica_server_address && strcmp(replica_server_address, server_address) != 0 ) {
        char replica_ip[256];
        int replica_port;
        sscanf(replica_server_address, "%[^:]:%d", replica_ip, &replica_port);

        int replica_socket;
        struct sockaddr_in replica_addr;

        replica_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (replica_socket < 0) {
            perror("Failed to create replica socket");
            return;
        }

        replica_addr.sin_family = AF_INET;
        replica_addr.sin_port = htons(replica_port);
        inet_pton(AF_INET, replica_ip, &replica_addr.sin_addr);

        if (connect(replica_socket, (struct sockaddr *)&replica_addr, sizeof(replica_addr)) < 0) {
            perror("Failed to connect to replica server");
            close(replica_socket);
            return;
        }

        if (strncmp(client_request, "set", 3) == 0) {
            send(replica_socket, client_request, strlen(client_request), 0);
        } else if (strncmp(client_request, "get", 3) == 0) {
            char replica_buffer[BUFFER_SIZE] = {0};
            send(replica_socket, client_request, strlen(client_request), 0);
            int replica_bytes_received = recv(replica_socket, replica_buffer, BUFFER_SIZE - 1, 0);

            if (replica_bytes_received > 0) {
                replica_buffer[replica_bytes_received] = '\0';

                char replica_response[BUFFER_SIZE];
                snprintf(replica_response, BUFFER_SIZE, "Replica Server: %s | Response: %s", replica_server_address, replica_buffer);
                send(client_socket, replica_response, strlen(replica_response), 0);
            }
        }
        else if (strncmp(client_request, "delete", 6) == 0) {
            send(replica_socket, client_request, strlen(client_request), 0);
        }

        close(replica_socket);
    }
}


void *handle_client(void *client_socket_ptr) {
    int client_socket = *(int *)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[BUFFER_SIZE] = {0};
    recv(client_socket, buffer, BUFFER_SIZE, 0);

    char command[10], key[256];
    sscanf(buffer, "%s %s", command, key);

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
            add_node(&ring, buffer);
            pthread_mutex_unlock(&lock);
            printf("Server '%s' added to the ring\n", buffer);
        }
    }

    close(announce_socket);
    return NULL;
}

int main() {
    pthread_mutex_init(&lock, NULL);

    pthread_t announce_thread, health_check_thread;
    pthread_create(&announce_thread, NULL, handle_server_announcement, NULL);
    pthread_create(&health_check_thread, NULL, health_check, NULL);

    int lb_socket, client_socket;
    struct sockaddr_in lb_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

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
