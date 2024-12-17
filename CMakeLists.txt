cmake_minimum_required(VERSION 3.30)
project(Distributed-Cache-System-main C)

set(CMAKE_C_STANDARD 11)

add_executable(Distributed-Cache-System-main main.c
        cache.h
        cache.c
        server.c
        client.c
        conhash.c
        conhash.h
        mockdb.h
        mockdb.c
        db_server.c
        load_balancer.c)