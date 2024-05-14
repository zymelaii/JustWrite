project(jwrite-test)

aux_source_directory(. SOURCE_FILES)

list(APPEND SOURCE_FILES ../src/TextViewEngine.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ../src)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets
	PRIVATE GTest::gtest
)
