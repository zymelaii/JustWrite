configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/Version.cpp.in
	${CMAKE_CURRENT_BINARY_DIR}/Version.cpp
)

file(GLOB CORE_SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
list(REMOVE_ITEM CORE_SOURCE_FILES App.cpp)

aux_source_directory(ui UI_SOURCE_FILES)
aux_source_directory(epub EPUB_SOURCE_FILES)

set(SOURCE_FILES App.cpp)
list(APPEND SOURCE_FILES ${UI_SOURCE_FILES})
list(APPEND SOURCE_FILES ${EPUB_SOURCE_FILES})
list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/res.qrc)
list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/JustWrite.rc)
list(APPEND SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp)

add_library(jwrite-core STATIC ${CORE_SOURCE_FILES})
target_link_libraries(
	jwrite-core
	PUBLIC Qt${QT_VERSION_MAJOR}::Gui
	PUBLIC cppjieba
	PUBLIC magic_enum::magic_enum
	PUBLIC spdlog::spdlog
	PUBLIC tomlplusplus::tomlplusplus
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
	${CMAKE_PROJECT_NAME}
	PRIVATE qt-material
	PRIVATE minizip
	PRIVATE widget-kit
	PRIVATE jwrite-core
	PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
	PRIVATE QWindowKit::Widgets
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE true)
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE "-O3")
endif()
