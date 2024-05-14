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

option(JWRITE_BUILD_TESTS "Build the unittests" ON)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

find_package(magic_enum QUIET)
if(NOT magic_enum_FOUND)
    set(MAGIC_ENUM_OPT_BUILD_EXAMPLES OFF)
    set(MAGIC_ENUM_OPT_BUILD_TESTS OFF)
    set(MAGIC_ENUM_OPT_INSTALL OFF)
    add_subdirectory(deps/magic_enum)
endif()

find_package(QWindowKit QUIET)
if(NOT QWindowKit_FOUND)
    set(QWINDOWKIT_INSTALL OFF)
    set(QWINDOWKIT_BUILD_STATIC ON)
    set(QWINDOWKIT_BUILD_EXAMPLES OFF)
    set(QWINDOWKIT_BUILD_EXAMPLES OFF)
    add_subdirectory(deps/qwindowskit)
endif()

if(JWRITE_BUILD_TESTS)
    find_package(GTest)
    if(NOT GTest_FOUND)
        message(WARNING "-- GTest not found, skip building tests")
        set(JWRITE_BUILD_TESTS OFF)
    endif()
endif()

set(INSTALL_TARGETS ${CMAKE_PROJECT_NAME})

add_subdirectory(src)

if(JWRITE_BUILD_TESTS)
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
