cmake_minimum_required(VERSION 4.1.2)
project(CBeancountParser VERSION 1.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 23)

find_package(Threads REQUIRED)

# debug
# set(CMAKE_BUILD_TYPE DEBUG)

include_directories(./include)

add_library(cbcparser SHARED
    src/files.c
    src/errors.c
    src/arithmetics.c
    src/date.c
    src/parser/lex.c
    src/parser/instances.c
    src/array.c
    src/hashmap.c
    src/models.c
)
target_link_libraries(cbcparser PRIVATE Threads::Threads)

# C-beancount-parser is intended to be a shared library.

add_executable(
   cbp
   src/main.c
)

target_link_libraries(cbp PRIVATE cbcparser)

