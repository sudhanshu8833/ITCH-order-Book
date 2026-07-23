cmake_minimum_required(VERSION 3.14)
project(itch_parser)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

find_package(benchmark REQUIRED)

add_executable(itch_parser 
            itch_parser.cpp
            parser_logic.cpp
            order_book_fast.cpp
)

target_link_libraries(itch_parser benchmark::benchmark)

