cmake_minimum_required(VERSION 3.14)
project(ServerWeb C)

set(CMAKE_C_STANDARD 99)

include_directories(Ezio/headers)

add_executable(ServerBranch Ezio/src/serverBranch.c)
target_link_libraries(ServerBranch pthread)
target_link_libraries(ServerBranch wurfl)
target_link_libraries(ServerBranch MagickWand)

add_executable(ServerBranchesHandler Ezio/src/serverBranchesHandler.c)
target_link_libraries(ServerBranchesHandler pthread)
target_link_libraries(ServerBranchesHandler wurfl)

#used to test the hash function
add_executable(HashTester Ezio/src/hashFunctionTester.c)

