cmake_minimum_required(VERSION 3.30)
project(strip_exif)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Wpedantic)

enable_testing()

add_executable(strip_exif strip_exif.cpp)

install(TARGETS strip_exif DESTINATION bin)
install(FILES test_image.jpg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(
    NAME strip_exif
    COMMAND strip_exif ${CMAKE_CURRENT_BINARY_DIR}/test_image.jpg
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

