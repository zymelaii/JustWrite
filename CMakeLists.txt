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
    LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(JWRITE_BUILD_TESTS "Build the unittests" ON)

set(QT_DEPENDENCY Core Gui Widgets StateMachine)
find_package(QT NAMES Qt6 COMPONENTS ${QT_DEPENDENCY} REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${QT_DEPENDENCY} REQUIRED)

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
    add_subdirectory(deps/qwindowskit)
endif()

set(ZLIB_BUILD_EXAMPLES OFF)
set(SKIP_INSTALL_ALL ON)
add_subdirectory(deps/zlib)

if(JWRITE_BUILD_TESTS)
    find_package(GTest)
    if(NOT GTest_FOUND)
        message(WARNING "-- GTest not found, skip building tests")
        set(JWRITE_BUILD_TESTS OFF)
    endif()
endif()

set(INSTALL_TARGETS ${CMAKE_PROJECT_NAME})

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)

if(JWRITE_BUILD_TESTS)
    add_subdirectory(test)
    list(APPEND INSTALL_TARGETS jwrite-test)
endif()

install(
	TARGETS ${INSTALL_TARGETS}
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/assets/fonts
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

if(WIN32)
    add_custom_target(
        install-runtime-deps
        COMMAND cmake -E copy $<TARGET_FILE:zlib> ${CMAKE_INSTALL_PREFIX}
        COMMENT "Install runtime dependencies..."
        DEPENDS zlib install
    )
    add_custom_target(
        deploy
        COMMAND windeployqt ${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:${CMAKE_PROJECT_NAME}>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:--debug>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:--release>
        COMMENT "Deploy the application..."
        DEPENDS install-runtime-deps
    )
else()
    add_custom_target(
        deploy
        COMMENT "Deploy the application..."
        DEPENDS install
    )
endif()
