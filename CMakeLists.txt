cmake_minimum_required(VERSION 3.5)

project(
    JustWrite
    VERSION 0.1.0
    DESCRIPTION "One of the most focused novel writer in the world!"
    HOMEPAGE_URL "https://github.com/zymelaii/JustWrite"
    LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(OpenGL REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory(src)

install(
	TARGETS ${CMAKE_PROJECT_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/assets/fonts
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
