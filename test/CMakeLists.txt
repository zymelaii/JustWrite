project(jwrite-test)

find_package(GTest REQUIRED)

aux_source_directory(. SOURCE_FILES)

list(APPEND SOURCE_FILES ../src/TextViewEngine.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE Qt6::Core Qt6::Widgets
	PRIVATE GTest::gtest GTest::gtest_main
)
