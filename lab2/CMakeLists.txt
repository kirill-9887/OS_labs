cmake_minimum_required(VERSION 3.10)
project(lab2)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lrt -Wall -Wextra -pedantic -Werror=maybe-uninitialized")

add_library(lab2_lib ./src/card_model.c ./src/time_conv.c)
add_executable(app ./app.c)
target_link_libraries(app PRIVATE lab2_lib)
