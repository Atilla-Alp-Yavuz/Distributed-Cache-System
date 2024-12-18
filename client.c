#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define LOAD_BALANCER_ADDRESS "127.0.0.1"
#define LOAD_BALANCER_PORT 9090

pthread_mutex_t lock;

// Function to send a single command to the load balancer
void send_to_load_balancer(const char *command) {
    int sock;
    struct sockaddr_in lb_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    lb_addr.sin_family = AF_INET;
    lb_addr.sin_port = htons(LOAD_BALANCER_PORT);
    inet_pton(AF_INET, LOAD_BALANCER_ADDRESS, &lb_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&lb_addr, sizeof(lb_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }

    send(sock, command, strlen(command), 0);
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Response to '%s': %s\n", command, buffer);
    }
    close(sock);
}

// Thread function for executing a single command
void *execute_command(void *arg) {
    char *command = (char *)arg;

    // Lock the mutex to ensure sequential execution
    pthread_mutex_lock(&lock);
    send_to_load_balancer(command);
    pthread_mutex_unlock(&lock);

    free(command);
    return NULL;
}

// Execute concurrent commands using threads
void execute_concurrent_commands(char *input) {
    pthread_t threads[BUFFER_SIZE / 10];
    int thread_count = 0;

    char *token = strtok(input, "&&");
    while (token) {
        while (*token == ' ') token++;  // Trim leading spaces

        char *command = malloc(strlen(token) + 1);
        strcpy(command, token);

        if (pthread_create(&threads[thread_count], NULL, execute_command, command) != 0) {
            perror("Failed to create thread");
            free(command);
        } else {
            thread_count++;
        }

        token = strtok(NULL, "&&");
    }

    // Wait for all threads to finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

int count_lines_in_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }

    int linecount = 0;
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        linecount++;
    }

    fclose(file);
    return linecount;
}

void process_batch_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open batch file");
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';  // Remove newline character
        if (strlen(line) == 0) continue;   // Skip empty lines

        if (strstr(line, "&&")) {
            execute_concurrent_commands(line);
        } else {
            pthread_mutex_lock(&lock);
            send_to_load_balancer(line);
            pthread_mutex_unlock(&lock);
        }
    }

    fclose(file);
}

int main() {
    // Initialize the mutex
    pthread_mutex_init(&lock, NULL);

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
        } else if (strncmp(input, "batch ", 6) == 0) {
            // Extract the filename
            char *filename = input + 6;
            printf("Processing batch file: %s\n", filename);

            // Count lines and process the file
            int linecount = count_lines_in_file(filename);
            printf("Total lines in file: %d\n", linecount);

            process_batch_file(filename);
        } else {
            send_to_load_balancer(input);
        }
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
