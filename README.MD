This project requires mutliple terminals to work and the system will be a representation of different processes that would normally represent a distributed cache system.

Commands:
To build the app:
make
To start the database server:
make run-db-server
To start the load balancer:
make run-load-balancer:

To start a server:
make run-server PORT=<port> (port can be any valid available port)

To start a client:
make run-client

Commands in the client:
get <keyvalue> (fetches the key from the database and saves it to the cache or if it already is in the cache, only fetches it from the cache)
delete <keyvalue> (deletes the key from all caches and the database)
set <keyvalue> <Value> (saves the value with the keyvalue to a cache and the database)
<function> && <function> (does the functions concurrently, can be more than two functions) (functions are get set delete)
batch <pathToBatchFile>.txt (Does the functions in the text file line by line)
