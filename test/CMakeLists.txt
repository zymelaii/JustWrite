project(jwrite-test)

aux_source_directory(. SOURCE_FILES)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE jwrite-core
	PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
	PRIVATE GTest::gtest
)
