#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define LOAD_BALANCER_ADDRESS "127.0.0.1" // Address of the load balancer
#define LOAD_BALANCER_PORT 9090           // Port of the load balancer

void send_to_load_balancer(const char *command) {
    int sock;
    struct sockaddr_in lb_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    lb_addr.sin_family = AF_INET;
    lb_addr.sin_port = htons(LOAD_BALANCER_PORT);

    // Convert address from text to binary form
    if (inet_pton(AF_INET, LOAD_BALANCER_ADDRESS, &lb_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(sock);
        return;
    }

    // Connect to the load balancer
    if (connect(sock, (struct sockaddr *)&lb_addr, sizeof(lb_addr)) < 0) {
        perror("Connection to load balancer failed");
        close(sock);
        return;
    }

    // Send command
    send(sock, command, strlen(command), 0);

    // Receive response
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Load Balancer Response: %s\n", buffer);
    }

    close(sock);
}

int main() {
    char command[BUFFER_SIZE];

    while (1) {
        printf("Enter command (e.g., set key value ttl, get key, delete key): ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = '\0'; // Remove newline character

        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Send command to the load balancer
        send_to_load_balancer(command);
    }

    return 0;
}

