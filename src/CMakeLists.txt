aux_source_directory(. SOURCE_FILES)
aux_source_directory(ui UI_SOURCE_FILES)

list(APPEND SOURCE_FILES ${UI_SOURCE_FILES})
list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/res.qrc)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
	${CMAKE_PROJECT_NAME}
	PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets
	PRIVATE magic_enum::magic_enum
	PRIVATE QWindowKit::Widgets
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE true)
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE "-O3")
endif()
