cmake_minimum_required(VERSION 2.8)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories($ENV{AMDAPPSDKROOT}/include)
link_directories($ENV{AMDAPPSDKROOT}/lib/x86)

if (WIN32)
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

add_subdirectory(ai-isa-jit)
add_subdirectory(test)
