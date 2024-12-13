# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread
SSLFLAGS = -lssl -lcrypto

# Source files
SERVER_SRC = server.c cache.c
CLIENT_SRC = client.c
LOAD_BALANCER_SRC = load_balancer.c conhash.c

# Output binaries
SERVER_BIN = server
CLIENT_BIN = client
LOAD_BALANCER_BIN = load_balancer

# Configuration file to store server ports
SERVER_CONFIG = servers.txt

# Default target: Build all components
all: $(SERVER_BIN) $(CLIENT_BIN) $(LOAD_BALANCER_BIN)

# Build the server
$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN) $(LDFLAGS)

# Build the client
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN) $(SSLFLAGS)

# Build the load balancer
$(LOAD_BALANCER_BIN): $(LOAD_BALANCER_SRC)
	$(CC) $(CFLAGS) $(LOAD_BALANCER_SRC) -o $(LOAD_BALANCER_BIN) $(LDFLAGS) $(SSLFLAGS)

# Run the server with a specified port
run-server:
	@if [ -z "$(PORT)" ]; then \
		echo "Usage: make run-server PORT=<port>"; \
		exit 1; \
	fi; \
	echo "127.0.0.1:$(PORT)" >> $(SERVER_CONFIG); \
	./$(SERVER_BIN) $(PORT)

# Run the load balancer
run-load-balancer:
	./$(LOAD_BALANCER_BIN)

# Run the client
run-client:
	./$(CLIENT_BIN)

# Clean up generated files
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(LOAD_BALANCER_BIN) $(SERVER_CONFIG)
