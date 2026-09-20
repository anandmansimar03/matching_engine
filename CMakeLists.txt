cmake_minimum_required(VERSION 3.20)
project(order_book_matching_engine LANGUAGES CXX)

add_library(order_book
    src/order_book.cpp
    src/order_pool.cpp)

target_include_directories(order_book PUBLIC src/include)
target_compile_features(order_book PUBLIC cxx_std_23)
target_compile_options(order_book PRIVATE -Wall -Wextra -Wpedantic)

add_executable(order_book_demo src/main.cpp)
target_link_libraries(order_book_demo PRIVATE order_book)
target_compile_options(order_book_demo PRIVATE -Wall -Wextra -Wpedantic)

get_filename_component(OB_CXX_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
set_target_properties(order_book_demo PROPERTIES
    BUILD_RPATH ${OB_CXX_DIR}/../lib64)
