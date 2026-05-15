cmake_minimum_required(VERSION 3.20)

project(ZIOVPOTrayApp LANGUAGES C CXX)

set(RPC_IDL ${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/tray_rpc.idl)
set(RPC_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(RPC_HEADER ${RPC_GEN_DIR}/tray_rpc.h)
set(RPC_CLIENT ${RPC_GEN_DIR}/tray_rpc_client.c)
set(RPC_SERVER ${RPC_GEN_DIR}/tray_rpc_server.c)

add_custom_command(
    OUTPUT ${RPC_HEADER} ${RPC_CLIENT} ${RPC_SERVER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RPC_GEN_DIR}
    COMMAND midl /nologo /env win64
            /h ${RPC_HEADER}
            /cstub ${RPC_CLIENT}
            /sstub ${RPC_SERVER}
            ${RPC_IDL}
    DEPENDS ${RPC_IDL}
    VERBATIM
)

set_source_files_properties(${RPC_CLIENT} ${RPC_SERVER} PROPERTIES GENERATED TRUE)
add_custom_target(GenerateRpc DEPENDS ${RPC_HEADER} ${RPC_CLIENT} ${RPC_SERVER})

add_executable(ZIOVPOTrayApp WIN32
    src/main.cpp
    src/rpc_client.cpp
    resources/app.rc
    ${RPC_CLIENT}
)

add_executable(ZIOVPOService
    src/service.cpp
    ${RPC_SERVER}
)

add_dependencies(ZIOVPOTrayApp GenerateRpc)
add_dependencies(ZIOVPOService GenerateRpc)

target_compile_features(ZIOVPOTrayApp PRIVATE cxx_std_17)
target_compile_features(ZIOVPOService PRIVATE cxx_std_17)
target_compile_definitions(ZIOVPOTrayApp PRIVATE UNICODE _UNICODE)
target_compile_definitions(ZIOVPOService PRIVATE UNICODE _UNICODE)

if(MSVC)
    target_compile_options(ZIOVPOTrayApp PRIVATE /utf-8)
    target_compile_options(ZIOVPOService PRIVATE /utf-8)
endif()

target_include_directories(ZIOVPOTrayApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RPC_GEN_DIR}
)

target_include_directories(ZIOVPOService PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RPC_GEN_DIR}
)

target_link_libraries(ZIOVPOTrayApp PRIVATE
    user32
    shell32
    gdi32
    advapi32
    rpcrt4
)

target_link_libraries(ZIOVPOService PRIVATE
    advapi32
    rpcrt4
    userenv
    wtsapi32
)
