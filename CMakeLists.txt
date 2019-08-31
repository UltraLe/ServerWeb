cmake_minimum_required(VERSION 3.14)
project(ServerWeb C)

set(CMAKE_C_STANDARD 99)

include_directories(Ezio/headers)
include_directories(/home/ezio/Scrivania/ImageMagick-7.0.8-62)

add_executable(ServerBranch Ezio/src/serverBranch.c)
target_link_libraries(ServerBranch pthread)
target_link_libraries(ServerBranch wurfl)

#target_link_libraries(ServerBranch MagickWand)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(GTK "MagickWand")
    if (GTK_FOUND)
        target_link_libraries(ServerBranch ${GTK_LIBRARIES})
        add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER})
    endif()
endif()

#file(GLOB SOURCES "/usr/local/include/ImageMagick-7/MagickWand/*.h")
#file(GLOB SOURCES1 "/usr/local/include/ImageMagick-7/MAgickCore/*.h")
#target_link_libraries(ServerBranch ${SOURCES})
#target_link_libraries(ServerBranch ${SOURCES1})

add_executable(ServerBranchesHandler Ezio/src/serverBranchesHandler.c)
target_link_libraries(ServerBranchesHandler pthread)

#used to test the hash function
add_executable(HashTester Ezio/src/hashFunctionTester.c)





