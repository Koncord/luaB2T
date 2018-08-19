cmake_minimum_required(VERSION 3.10)
project(B2T)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

find_package(LuaJit REQUIRED)

include(GenerateExportHeader)

include_directories(SYSTEM "."
        "${LuaJit_INCLUDE_DIRS}"
        "${CMAKE_SOURCE_DIR}/extern/json/include"
        "${CMAKE_SOURCE_DIR}/extern/sol/single"
        "${PROJECT_BINARY_DIR}")

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_executable(B2T library.cpp)
    target_compile_definitions(B2T PRIVATE _DEBUG)
else()
    add_library(B2T SHARED library.cpp library.h)
    generate_export_header(B2T)
    set_target_properties(B2T PROPERTIES PREFIX "")
endif()
target_link_libraries(B2T ${LuaJit_LIBRARIES})
