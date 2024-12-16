cmake_minimum_required(VERSION 3.30)
project(ybgdon C)

set(CMAKE_C_STANDARD 11)

add_executable(ybgdon main.c
        cache.h
        cache.c
        server.c
        client.c
        conhash.c
        conhash.h
        load_balancer.c
        mock_db.h
        mock_db.c)
