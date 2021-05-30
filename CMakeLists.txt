cmake_minimum_required(VERSION 3.17.0)
project(shell)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_BUILD_TYPE Debug)

add_executable(shell "")
add_subdirectory(src)

set(CXX_COMPILE_FLAGS)
set(CXX_LINK_FLAGS)
list(APPEND CXX_COMPILE_FLAGS -Wall -Wextra -pedantic -Wno-narrowing -Wno-sign-compare)
list(APPEND CXX_LINK_FLAGS -fuse-ld=gold )

if(CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND CXX_COMPILE_FLAGS -fno-omit-frame-pointer
            -fno-optimize-sibling-calls -fsanitize=address)
    list(APPEND CXX_LINK_FLAGS -fsanitize=address
            -fno-omit-frame-pointer -fno-optimize-sibling-calls)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    list(APPEND CXX_COMPILE_FLAGS -O3)
endif()

set(CXX_INCLUDE_DIRS)
set(CXX_LINK_LIBS)
list(APPEND CXX_INCLUDE_DIRS "include")
list(APPEND CXX_LINK_LIBS fmt::fmt)

find_package(fmt)

# Boost stuff
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS filesystem)

if(Boost_FOUND)
    list(APPEND CXX_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
    list(APPEND CXX_LINK_LIBS ${Boost_LIBRARIES})
endif()

target_compile_options(shell PRIVATE ${CXX_COMPILE_FLAGS})
target_link_options(shell PRIVATE ${CXX_LINK_FLAGS})
target_include_directories(shell PRIVATE SYSTEM
        ${CXX_INCLUDE_DIRS})
target_link_libraries(shell ${CXX_LINK_LIBS})
