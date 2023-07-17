cmake_minimum_required(VERSION 3.0)
project(CPPNETWORK)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 17)
# set out path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# set .so path
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# set .cpp path
file(GLOB SRC_LIST1 ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/service/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rpc_server_main.cpp
        )
file(GLOB SRC_LIST2 ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/service/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rpc_client_main.cpp
        )
file(GLOB SRC_LIST3 ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/service/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/http_server_main.cpp
        )
file(GLOB SRC_LIST4 ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/service/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/registry_main.cpp
        )
# print SRC_LIST
#message("SRC_LIST = ${SRC_LIST1}")
#message("SRC_LIST = ${SRC_LIST2}")
# set .h path
include_directories(${PROJECT_SOURCE_DIR}/include/common
        ${PROJECT_SOURCE_DIR}/include/taskflow
        ${PROJECT_SOURCE_DIR}/include/service)

add_executable(exeserver ${SRC_LIST1} )
add_executable(execlient ${SRC_LIST2})
add_executable(exeserver_http ${SRC_LIST3})
add_executable(exeregistry ${SRC_LIST4})
target_link_libraries(exeserver pthread)
target_link_libraries(execlient pthread)
target_link_libraries(exeserver_http pthread)
target_link_libraries(exeregistry pthread)
