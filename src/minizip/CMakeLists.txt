project(minizip)

aux_source_directory(. SOURCE_FILES)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(${PROJECT_NAME} PUBLIC zlib)
