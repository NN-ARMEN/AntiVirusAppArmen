cmake_minimum_required(VERSION 3.15)
project(AVAA VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

add_executable(AVAA WIN32 src/main.cpp)

if(MSVC)
    target_compile_options(AVAA PRIVATE /W4)
    target_compile_definitions(AVAA PRIVATE _UNICODE UNICODE)
endif()