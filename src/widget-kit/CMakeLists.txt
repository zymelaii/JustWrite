project(widget-kit)

aux_source_directory(. SOURCE_FILES)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
target_link_libraries(
	${PROJECT_NAME}
	PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui
    PRIVATE qt-material
)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)
