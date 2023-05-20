cmake_minimum_required(VERSION 3.24)
project(project2 C)

set(CMAKE_C_STANDARD 11)

add_executable(
        project2
        src
        client-server
        include)

target_include_directories(project2 PUBLIC include/)