cmake_minimum_required(VERSION 3.16.3)
project(pa3-2 C)

set(CMAKE_C_STANDARD 99)
#add_compile_options(-Wall -Wextra -pedantic -Werror)

add_library(rn SHARED IMPORTED)
set_target_properties(rn PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libruntime.so"
        )
#add_executable(lab3 pa31/pa23.c pa31/bank_robbery.c)
add_executable(pa3 pa23.c bank_robbery.c)
target_link_libraries(pa3 rn)
