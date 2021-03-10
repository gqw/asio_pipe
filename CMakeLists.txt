cmake_minimum_required(VERSION 3.6)

project(asio_pipe)

file(GLOB HEADER_FILES ./src/*.h)
file(GLOB SOURCE_FILES ./src/*.cpp)

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})