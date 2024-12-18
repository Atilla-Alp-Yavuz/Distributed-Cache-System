#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define LOAD_BALANCER_ADDRESS "127.0.0.1"
#define LOAD_BALANCER_PORT 9090
#define MAX_SERVERS 5
#define MAX_CLIENTS 5
#define SERVER_START_PORT 8080
#define LOG_FILE "server.log"

GtkWidget *response_textview, *server_output_textview, *command_entry, *client_menu;
GtkTextBuffer *response_buffer, *server_buffer;
int server_count = 0;
int client_count = 0;
pid_t server_pids[MAX_SERVERS];
pid_t client_pids[MAX_CLIENTS];
pid_t load_balancer_pid = -1;
pid_t db_server_pid = -1;
char client_list[MAX_CLIENTS][20];
long last_log_position = 0; // To track the last read position in the log file

// Append text to a GTK text buffer
void append_text_to_buffer(GtkTextBuffer *buffer, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1); // Add a newline
}

// Clean up processes and resources on exit
void cleanup_on_exit() {
    for (int i = 0; i < server_count; i++) {
        if (server_pids[i] > 0) {
            kill(server_pids[i], SIGTERM);
            printf("Terminated server with PID %d\n", server_pids[i]);
        }
    }
    for (int i = 0; i < client_count; i++) {
        if (client_pids[i] > 0) {
            kill(client_pids[i], SIGTERM);
            printf("Terminated client with PID %d\n", client_pids[i]);
        }
    }
    if (load_balancer_pid > 0) {
        kill(load_balancer_pid, SIGTERM);
        printf("Terminated load balancer with PID %d\n", load_balancer_pid);
    }
    if (db_server_pid > 0) {
        kill(db_server_pid, SIGTERM);
        printf("Terminated database server with PID %d\n", db_server_pid);
    }
    printf("All processes terminated. Exiting application.\n");
}

// Start load balancer and database server
void start_load_balancer_and_db_server() {
    db_server_pid = fork();
    if (db_server_pid == 0) {
        execl("./db_server", "./db_server", NULL);
        perror("Failed to launch database server");
        exit(EXIT_FAILURE);
    } else if (db_server_pid < 0) {
        perror("Failed to fork database server process");
    } else {
        append_text_to_buffer(response_buffer, "Database server started.");
    }

    load_balancer_pid = fork();
    if (load_balancer_pid == 0) {
        execl("./load_balancer", "./load_balancer", NULL);
        perror("Failed to launch load balancer");
        exit(EXIT_FAILURE);
    } else if (load_balancer_pid < 0) {
        perror("Failed to fork load balancer process");
    } else {
        append_text_to_buffer(response_buffer, "Load balancer started.");
    }
}

// Read logs from server.log and update the Server Output TextView
void read_server_logs() {
    FILE *log_file = fopen(LOG_FILE, "r");
    if (log_file == NULL) {
        append_text_to_buffer(server_buffer, "Error: Failed to open server.log file.");
        return;
    }

    // Move to the last read position in the log file
    fseek(log_file, last_log_position, SEEK_SET);
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), log_file)) {
        // Remove trailing newline characters for clean output
        line[strcspn(line, "\r\n")] = 0;
        append_text_to_buffer(server_buffer, line);
    }
    last_log_position = ftell(log_file); // Update last read position
    fclose(log_file);
}

// Periodically update server logs
gboolean update_logs(gpointer data) {
    read_server_logs();
    return TRUE; // Continue running the timeout function
}

// Handle "Create Server" button click
void on_create_server_clicked(GtkWidget *widget, gpointer data) {
    if (server_count >= MAX_SERVERS) {
        append_text_to_buffer(response_buffer, "Maximum server limit reached (5).");
        return;
    }

    int port = SERVER_START_PORT + server_count;
    char port_str[10];
    snprintf(port_str, sizeof(port_str), "%d", port);

    pid_t pid = fork();
    if (pid == 0) {
        char *server_args[] = {"./server", port_str, NULL};
        execvp("./server", server_args);
        perror("Failed to launch server");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        server_pids[server_count] = pid;
        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "Server %d started on port %d.", server_count + 1, port);
        append_text_to_buffer(response_buffer, message);
        server_count++;
    }
}

// Handle "Create Client" button click
void on_create_client_clicked(GtkWidget *widget, gpointer data) {
    if (client_count >= MAX_CLIENTS) {
        append_text_to_buffer(response_buffer, "Maximum client limit reached (5).");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        char *client_args[] = {"./client", NULL};
        execvp("./client", client_args);
        perror("Failed to launch client");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        client_pids[client_count] = pid;
        snprintf(client_list[client_count], sizeof(client_list[client_count]), "Client %d", client_count + 1);
        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "Client %d started.", client_count + 1);
        append_text_to_buffer(response_buffer, message);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(client_menu), client_list[client_count]);
        client_count++;
    }
}

// Handle "Send Command" button click
void on_send_command_clicked(GtkWidget *widget, gpointer data) {
    const char *selected_client = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(client_menu));
    const char *command = gtk_entry_get_text(GTK_ENTRY(command_entry));

    if (!selected_client || strlen(command) == 0) {
        append_text_to_buffer(response_buffer, "Please select a client and enter a command.");
        return;
    }

    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Sending to %s: %s", selected_client, command);
    append_text_to_buffer(response_buffer, message);

    snprintf(message, BUFFER_SIZE, "Response: Command '%s' sent to %s", command, selected_client);
    append_text_to_buffer(response_buffer, message);
}

// Main function to initialize the GUI
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Cache System GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(cleanup_on_exit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    response_buffer = gtk_text_buffer_new(NULL);
    server_buffer = gtk_text_buffer_new(NULL);

    client_menu = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(vbox), client_menu, FALSE, FALSE, 5);

    command_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(command_entry), "Enter command (e.g., set key value ttl)");
    gtk_box_pack_start(GTK_BOX(vbox), command_entry, FALSE, FALSE, 5);

    GtkWidget *send_button = gtk_button_new_with_label("Send Command");
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 5);
    g_signal_connect(send_button, "clicked", G_CALLBACK(on_send_command_clicked), NULL);

    response_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(response_textview), FALSE);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(response_textview), response_buffer);

    GtkWidget *response_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(response_scrolled), response_textview);
    gtk_box_pack_start(GTK_BOX(vbox), response_scrolled, TRUE, TRUE, 5);

    server_output_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(server_output_textview), FALSE);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(server_output_textview), server_buffer);

    GtkWidget *server_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(server_scrolled), server_output_textview);
    gtk_box_pack_start(GTK_BOX(vbox), server_scrolled, TRUE, TRUE, 5);

    GtkWidget *create_server_button = gtk_button_new_with_label("Create Server");
    gtk_box_pack_start(GTK_BOX(vbox), create_server_button, FALSE, FALSE, 5);
    g_signal_connect(create_server_button, "clicked", G_CALLBACK(on_create_server_clicked), NULL);

    GtkWidget *create_client_button = gtk_button_new_with_label("Create Client");
    gtk_box_pack_start(GTK_BOX(vbox), create_client_button, FALSE, FALSE, 5);
    g_signal_connect(create_client_button, "clicked", G_CALLBACK(on_create_client_clicked), NULL);

    gtk_widget_show_all(window);

    start_load_balancer_and_db_server();

    g_timeout_add(1000, update_logs, NULL); // Periodically update server logs

    gtk_main();

    cleanup_on_exit();
    return 0;
}
