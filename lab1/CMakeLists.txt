cmake_minimum_required(VERSION 3.10)
project(lab1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror=maybe-uninitialized")

add_executable(client ./src/client.c)
add_executable(server ./src/server.c)
