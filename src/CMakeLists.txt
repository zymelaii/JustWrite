aux_source_directory(. SOURCE_FILES)

list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/res.qrc)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
	${CMAKE_PROJECT_NAME}
	PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets
)
