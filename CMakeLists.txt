cmake_minimum_required(VERSION 3.20)

project(ipnum
    VERSION 1.0.0
    DESCRIPTION "Converts IPv4 and IPv6 addresses to their numeric value"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(IPNUM_BUILD_TESTS "Build the unit tests" ON)
option(IPNUM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(ipnum_warnings INTERFACE)
if(MSVC)
    target_compile_options(ipnum_warnings INTERFACE /W4 /permissive- /utf-8)
    target_compile_definitions(ipnum_warnings INTERFACE _CRT_SECURE_NO_WARNINGS)
    if(IPNUM_WARNINGS_AS_ERRORS)
        target_compile_options(ipnum_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(ipnum_warnings INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(IPNUM_WARNINGS_AS_ERRORS)
        target_compile_options(ipnum_warnings INTERFACE -Werror)
    endif()
endif()

add_library(ipnum_core STATIC
    src/cli.cpp
    src/format.cpp
    src/ipv4_address.cpp
    src/ipv6_address.cpp
    src/parse_result.cpp
    src/uint128.cpp)
add_library(ipnum::core ALIAS ipnum_core)
target_include_directories(ipnum_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ipnum_core PRIVATE ipnum_warnings)

add_executable(ipnum app/main.cpp)
target_compile_definitions(ipnum PRIVATE IPNUM_VERSION="${PROJECT_VERSION}")
target_link_libraries(ipnum PRIVATE ipnum::core ipnum_warnings)
if(MINGW)
    target_link_options(ipnum PRIVATE -static)
endif()

if(IPNUM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS ipnum RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

string(TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" IPNUM_PLATFORM)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${IPNUM_PLATFORM}")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
