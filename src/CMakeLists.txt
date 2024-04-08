aux_source_directory(. SOURCE_FILES)

list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/res.qrc)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
	${CMAKE_PROJECT_NAME}
	PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE true)
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE "-O3")
endif()
