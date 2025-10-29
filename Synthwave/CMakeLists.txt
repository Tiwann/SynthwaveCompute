file(GLOB_RECURSE SYNTHWAVE_SOURCES Source/*.cpp Source/*.h)

add_executable(Synthwave)
set_target_properties(Synthwave PROPERTIES CXX_STANDARD 23)
target_sources(Synthwave PRIVATE ${SYNTHWAVE_SOURCES})
target_link_libraries(Synthwave PUBLIC NovaEngine)
target_compile_definitions(Synthwave PRIVATE NOVA_APPLICATION_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES MinSizeRel)
    set_target_properties(Synthwave PROPERTIES WIN32_EXECUTABLE TRUE)
endif ()