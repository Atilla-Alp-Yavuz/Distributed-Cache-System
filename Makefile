# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread
SSLFLAGS = -lssl -lcrypto

# Source files
SERVER_SRC = server.c cache.c
CLIENT_SRC = client.c
LOAD_BALANCER_SRC = load_balancer.c conhash.c
DB_SERVER_SRC = db_server.c mockdb.c

# Output binaries
SERVER_BIN = server
CLIENT_BIN = client
LOAD_BALANCER_BIN = load_balancer
DB_SERVER_BIN = db_server

# Configuration file to store server ports
SERVER_CONFIG = servers.txt

# Header files (for dependency tracking)
HEADERS = cache.h mockdb.h

# Default target: Build all components
all: $(SERVER_BIN) $(CLIENT_BIN) $(LOAD_BALANCER_BIN) $(DB_SERVER_BIN)

# Build the server
$(SERVER_BIN): $(SERVER_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN) $(LDFLAGS)

# Build the client
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN) $(SSLFLAGS)

# Build the load balancer
$(LOAD_BALANCER_BIN): $(LOAD_BALANCER_SRC)
	$(CC) $(CFLAGS) $(LOAD_BALANCER_SRC) -o $(LOAD_BALANCER_BIN) $(LDFLAGS) $(SSLFLAGS)

# Build the database server
$(DB_SERVER_BIN): $(DB_SERVER_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(DB_SERVER_SRC) -o $(DB_SERVER_BIN)

# Run the database server
run-db-server:
	./$(DB_SERVER_BIN)

# Run the cache server with a specified port
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
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(LOAD_BALANCER_BIN) $(DB_SERVER_BIN) $(SERVER_CONFIG)
