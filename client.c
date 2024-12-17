#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define LOAD_BALANCER_ADDRESS "127.0.0.1" // Address of the load balancer
#define LOAD_BALANCER_PORT 9090          // Port of the load balancer

// Function to send a single command to the load balancer
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
        printf("Response to '%s': %s\n", command, buffer);
    } else {
        printf("No response for '%s'\n", command);
    }

    close(sock);
}

// Thread function for concurrent command execution
void *execute_command(void *arg) {
    char *command = (char *)arg;
    send_to_load_balancer(command);
    free(command);
    return NULL;
}

// Split input by '&&' and execute commands concurrently
void execute_concurrent_commands(char *input) {
    char *token;
    pthread_t threads[BUFFER_SIZE / 10]; // Max thread limit
    int thread_count = 0;

    // Tokenize input based on "&&"
    token = strtok(input, "&&");
    while (token != NULL) {
        while (*token == ' ') token++; // Trim leading spaces

        // Copy the command and create a thread to execute it
        char *command = malloc(BUFFER_SIZE);
        strncpy(command, token, BUFFER_SIZE - 1);

        if (pthread_create(&threads[thread_count], NULL, execute_command, (void *)command) != 0) {
            perror("Failed to create thread");
            free(command);
        } else {
            thread_count++;
        }

        token = strtok(NULL, "&&");
    }

    // Wait for all threads to complete
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main() {
    char input[BUFFER_SIZE];

    while (1) {
        printf("Enter command (use '&&' for concurrency, 'exit' to quit): ");
        fgets(input, BUFFER_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0'; // Remove newline character

        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Check for concurrent commands
        if (strstr(input, "&&") != NULL) {
            execute_concurrent_commands(input);
        } else {
            send_to_load_balancer(input);
        }
    }

    return 0;
}
