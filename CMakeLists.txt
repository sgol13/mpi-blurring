cmake_minimum_required(VERSION 3.10.2)

project(mpi-blurring)

find_package(MPI REQUIRED)

file(GLOB SOURCES src/*.cpp)
add_executable(exe ${SOURCES})

target_include_directories(exe PRIVATE ${MPI_CXX_INCLUDE_PATH} include/)

target_compile_options(exe PRIVATE ${MPI_CXX_COMPILE_FLAGS})

target_link_libraries(exe ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS})
