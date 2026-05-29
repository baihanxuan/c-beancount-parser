cmake_minimum_required(VERSION 4.1.2)
project(CBeancountParser VERSION 1.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 23)

find_package(Threads REQUIRED)

# debug
# set(CMAKE_BUILD_TYPE DEBUG)

include_directories(./include)

add_library(cbp_lib SHARED
    src/array.c
    src/object.c
    src/parser.c
    src/files.c
    src/hashmap.c
    src/errors.c
    src/arithmetics.c
    src/date.c
    src/models.c
)
target_link_libraries(cbp_lib PRIVATE Threads::Threads)

# C-beancount-parser is intended to be a shared library.

# add_executable(
#     cbp
#     src/main.c
# )

# target_link_libraries(cbp PRIVATE cbp_lib)

