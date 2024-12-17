#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define LOAD_BALANCER_ADDRESS "127.0.0.1"
#define LOAD_BALANCER_PORT 9090
#define MAX_SERVERS 5
#define MAX_CLIENTS 5
#define SERVER_START_PORT 8080

GtkWidget *response_textview, *server_output_textview, *command_entry, *client_menu;
GtkTextBuffer *response_buffer, *server_buffer;
int server_count = 0;
int client_count = 0;
char client_list[MAX_CLIENTS][20];

void append_text_to_buffer(GtkTextBuffer *buffer, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);
}

void launch_process(const char *command, char *const argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(command, argv);
        perror("Failed to launch process");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Fork failed");
    }
}

void start_load_balancer_and_dbserver() {
    char *dbserver_args[] = {"./db_server", NULL};
    launch_process("./db_server", dbserver_args);
    append_text_to_buffer(response_buffer, "Database server started.");

    char *load_balancer_args[] = {"./load_balancer", NULL};
    launch_process("./load_balancer", load_balancer_args);
    append_text_to_buffer(response_buffer, "Load balancer started.");
}

void process_server_response(const char *response) {
    if (strstr(response, "Cache Hit") || strstr(response, "Cache Miss")) {
        append_text_to_buffer(server_buffer, response);
    }
}

void send_command_to_client(const char *client, const char *command) {
    int sock;
    struct sockaddr_in lb_addr;
    char response[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    lb_addr.sin_family = AF_INET;
    lb_addr.sin_port = htons(LOAD_BALANCER_PORT);

    if (inet_pton(AF_INET, LOAD_BALANCER_ADDRESS, &lb_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&lb_addr, sizeof(lb_addr)) < 0) {
        perror("Connection to load balancer failed");
        close(sock);
        return;
    }

    send(sock, command, strlen(command), 0);
    int bytes_received = recv(sock, response, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        append_text_to_buffer(response_buffer, response);
        process_server_response(response);
    } else {
        append_text_to_buffer(response_buffer, "No response received");
    }
    close(sock);
}

void on_create_server_clicked(GtkWidget *widget, gpointer data) {
    if (server_count >= MAX_SERVERS) {
        append_text_to_buffer(response_buffer, "Maximum server limit reached (5).");
        return;
    }

    int port = SERVER_START_PORT + server_count;
    char port_str[10];
    snprintf(port_str, sizeof(port_str), "%d", port);

    char *server_args[] = {"./server", port_str, NULL};
    launch_process("./server", server_args);

    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Server %d started on port %d.", server_count + 1, port);
    append_text_to_buffer(response_buffer, message);
    server_count++;
}

void on_create_client_clicked(GtkWidget *widget, gpointer data) {
    if (client_count >= MAX_CLIENTS) {
        append_text_to_buffer(response_buffer, "Maximum client limit reached (5).");
        return;
    }

    snprintf(client_list[client_count], sizeof(client_list[client_count]), "Client %d", client_count + 1);
    char *client_args[] = {"./client", NULL};
    launch_process("./client", client_args);

    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Client %d started.", client_count + 1);
    append_text_to_buffer(response_buffer, message);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(client_menu), client_list[client_count]);
    client_count++;
}

void on_send_command_clicked(GtkWidget *widget, gpointer data) {
    const char *selected_client = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(client_menu));
    const char *command = gtk_entry_get_text(GTK_ENTRY(command_entry));

    // Check for empty inputs
    if (!selected_client || strlen(selected_client) == 0) {
        append_text_to_buffer(response_buffer, "No client selected.");
        return;
    }
    if (!command || strlen(command) == 0 || strspn(command, " \t\n") == strlen(command)) {
        append_text_to_buffer(response_buffer, "Invalid command: Input cannot be empty.");
        return;
    }

    char message[BUFFER_SIZE];
    //snprintf(message, BUFFER_SIZE, "Sending to %s: %s", selected_client, command);
    append_text_to_buffer(response_buffer, message);

    // Send valid command
    send_command_to_client(selected_client, command);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Window setup
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Cache System GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Layouts
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Start load balancer and db server
    response_buffer = gtk_text_buffer_new(NULL);
    server_buffer = gtk_text_buffer_new(NULL);
    start_load_balancer_and_dbserver();

    // Client Menu
    client_menu = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(vbox), client_menu, FALSE, FALSE, 5);

    // Command input and send button
    command_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(command_entry), "Enter command (e.g., set key value ttl)");
    gtk_box_pack_start(GTK_BOX(vbox), command_entry, FALSE, FALSE, 5);

    GtkWidget *send_button = gtk_button_new_with_label("Send Command");
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 5);
    g_signal_connect(send_button, "clicked", G_CALLBACK(on_send_command_clicked), NULL);

    // Response display
    response_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(response_textview), FALSE);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(response_textview), response_buffer);

    GtkWidget *response_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(response_scrolled), response_textview);
    gtk_box_pack_start(GTK_BOX(vbox), response_scrolled, TRUE, TRUE, 5);

    // Server output display
    server_output_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(server_output_textview), FALSE);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(server_output_textview), server_buffer);

    GtkWidget *server_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(server_scrolled), server_output_textview);
    gtk_box_pack_start(GTK_BOX(vbox), server_scrolled, TRUE, TRUE, 5);

    // Buttons to create servers and clients
    GtkWidget *create_server_button = gtk_button_new_with_label("Create Server");
    gtk_box_pack_start(GTK_BOX(vbox), create_server_button, FALSE, FALSE, 5);
    g_signal_connect(create_server_button, "clicked", G_CALLBACK(on_create_server_clicked), NULL);

    GtkWidget *create_client_button = gtk_button_new_with_label("Create Client");
    gtk_box_pack_start(GTK_BOX(vbox), create_client_button, FALSE, FALSE, 5);
    g_signal_connect(create_client_button, "clicked", G_CALLBACK(on_create_client_clicked), NULL);

    // Show all widgets
    gtk_widget_show_all(window);
    gtk_main();
    cleanup_on_exit();

    return 0;
}
void cleanup_on_exit() {
    system("pkill load_balancer");
    system("pkill db_server");
    system("pkill server");
}
