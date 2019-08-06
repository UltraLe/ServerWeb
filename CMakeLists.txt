cmake_minimum_required(VERSION 3.14)
project(ServerWeb C)

set(CMAKE_C_STANDARD 99)

include_directories(Ezio/headers)

add_executable(ServerBranch Ezio/src/serverBranch.c Ezio/headers/writerThread.h)
target_link_libraries(ServerBranch pthread)

add_executable(ServerBranchesHandler Ezio/src/serverBranchesHandler.c)
target_link_libraries(ServerBranchesHandler pthread)
