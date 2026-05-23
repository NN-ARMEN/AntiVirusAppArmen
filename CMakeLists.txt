cmake_minimum_required(VERSION 3.20)

project(AVAA LANGUAGES C CXX)

find_program(MIDL_COMPILER midl)
if(NOT MIDL_COMPILER)
    file(GLOB SDK_BIN_DIRS
        "C:/Program Files (x86)/Windows Kits/10/bin/10.*/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/x64"
    )
    find_program(MIDL_COMPILER midl PATHS ${SDK_BIN_DIRS})
endif()
if(NOT MIDL_COMPILER)
    message(FATAL_ERROR "MIDL compiler not found. Install the Windows SDK.")
endif()

set(RPC_IDL "${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/avaa_rpc.idl")
set(RPC_ACF "${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/avaa_rpc.acf")
set(RPC_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/rpc_gen")
file(MAKE_DIRECTORY "${RPC_GEN_DIR}")

set(RPC_HEADER "${RPC_GEN_DIR}/avaa_rpc_h.h")
set(RPC_CLIENT "${RPC_GEN_DIR}/avaa_rpc_c.c")
set(RPC_SERVER "${RPC_GEN_DIR}/avaa_rpc_s.c")

add_custom_command(
    OUTPUT ${RPC_HEADER} ${RPC_CLIENT} ${RPC_SERVER}
    COMMAND "${MIDL_COMPILER}"
            /acf "${RPC_ACF}"
            /header avaa_rpc_h.h
            /cstub avaa_rpc_c.c
            /sstub avaa_rpc_s.c
            /env x64
            ${RPC_IDL}
    DEPENDS ${RPC_IDL} ${RPC_ACF}
    WORKING_DIRECTORY "${RPC_GEN_DIR}"
    COMMENT "Running MIDL compiler on avaa_rpc.idl"
    VERBATIM
)

add_custom_target(GenerateRpc DEPENDS ${RPC_HEADER} ${RPC_CLIENT} ${RPC_SERVER})

add_executable(AVAA WIN32
    src/main.cpp
    src/rpc_client.cpp
    resources/app.rc
    ${RPC_CLIENT}
)

add_executable(AVAAService
    src/service.cpp
    ${RPC_SERVER}
)

add_executable(AVAAMockLicenseServer
    src/mock_license_server.cpp
)

add_dependencies(AVAA GenerateRpc)
add_dependencies(AVAAService GenerateRpc)

target_compile_features(AVAA PRIVATE cxx_std_17)
target_compile_features(AVAAService PRIVATE cxx_std_17)
target_compile_features(AVAAMockLicenseServer PRIVATE cxx_std_17)
target_compile_definitions(AVAA PRIVATE UNICODE _UNICODE)
target_compile_definitions(AVAAService PRIVATE UNICODE _UNICODE)

if(MSVC)
    target_compile_options(AVAA PRIVATE /utf-8)
    target_compile_options(AVAAService PRIVATE /utf-8)
    target_compile_options(AVAAMockLicenseServer PRIVATE /utf-8)
endif()

target_include_directories(AVAA PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RPC_GEN_DIR}
)

target_include_directories(AVAAService PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RPC_GEN_DIR}
)

target_link_libraries(AVAA PRIVATE
    user32
    shell32
    comdlg32
    ole32
    gdi32
    advapi32
    rpcrt4
    winmm
)

target_link_libraries(AVAAService PRIVATE
    advapi32
    rpcrt4
    userenv
    winhttp
    wtsapi32
    bcrypt
)

target_link_libraries(AVAAMockLicenseServer PRIVATE
    ws2_32
)