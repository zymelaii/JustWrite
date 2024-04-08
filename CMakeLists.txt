cmake_minimum_required(VERSION 3.5)

file(READ "VERSION" APP_VERSION)
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${APP_VERSION})
set(MAJOR_VERSION ${CMAKE_MATCH_1})
set(MINOR_VERSION ${CMAKE_MATCH_2})
set(PATCH_VERSION ${CMAKE_MATCH_3})
set(BUILD_NUMBER ${CMAKE_MATCH_4})
math(EXPR BUILD_NUMBER "${BUILD_NUMBER} + 1")
set(APP_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}.${BUILD_NUMBER}")
file(WRITE "VERSION" "${APP_VERSION}")

project(
    JustWrite
    VERSION "${APP_VERSION}"
    DESCRIPTION "One of the most focused novel writer in the world!"
    HOMEPAGE_URL "https://github.com/zymelaii/JustWrite"
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(BUILD_TEST "Build the unittests" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(BUILD_TEST OFF)
endif()

set(INSTALL_TARGETS ${CMAKE_PROJECT_NAME})

add_subdirectory(src)

if(BUILD_TEST)
    add_subdirectory(test)
    list(APPEND INSTALL_TARGETS jwrite-test)
endif()

install(
	TARGETS ${INSTALL_TARGETS}
	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/assets/fonts
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
