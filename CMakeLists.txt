cmake_minimum_required(VERSION 3.15)
project(AVAA VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Графическое приложение (с rpc_client.cpp)
add_executable(AVAA WIN32
    src/main.cpp
    src/rpc_client.cpp
    src/rpc_interface.h
)

# Windows-служба
add_executable(AVAAService
    src/service.cpp
    src/rpc_server.cpp
    src/rpc_interface.h
)

# Подключение библиотек для AVAA
target_link_libraries(AVAA
    user32
    shell32
    advapi32
    rpcrt4
)

# Подключение библиотек для службы
target_link_libraries(AVAAService
    advapi32
    userenv
    wtsapi32
    rpcrt4
)

if(MSVC)
    target_compile_options(AVAA PRIVATE /W4)
    target_compile_options(AVAAService PRIVATE /W4)
    target_compile_definitions(AVAA PRIVATE _UNICODE UNICODE)
    target_compile_definitions(AVAAService PRIVATE _UNICODE UNICODE)
endif()