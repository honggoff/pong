cmake_minimum_required(VERSION 3.28)

project(pong
      LANGUAGES CXX
      HOMEPAGE_URL https://github.com/honggoff/pong
      DESCRIPTION "A Tiny Framebuffer Pong"
      VERSION "0.0.1"
      )

set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Debug)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG}")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wno-unused-parameter")

set(CMAKE_CXX_STANDARD 17)

add_executable(pong pong.cpp)

add_subdirectory(tfblib)

target_link_libraries(pong tfb)

add_executable(test test.cpp)
