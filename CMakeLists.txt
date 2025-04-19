cmake_minimum_required(VERSION 3.10)
project(RubiksCube)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define source files
set(SOURCES 
    main.cpp
    InitShader.cpp
    Cubie.cpp
    RubiksCube.cpp
)

# Create executable
add_executable(rubiks_cube ${SOURCES})

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# IMPORTANT: Include the "include" directory where Angel.h and other headers are located
target_include_directories(rubiks_cube PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
)

# For Apple Silicon, handle macOS specific frameworks
if(APPLE)
    target_link_libraries(rubiks_cube PRIVATE 
        ${OPENGL_LIBRARIES}
        glfw
        "-framework OpenGL" 
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreVideo"
    )
else()
    # Linux/Windows handling
    find_package(GLEW REQUIRED)
    target_link_libraries(rubiks_cube PRIVATE 
        ${OPENGL_LIBRARIES}
        glfw
        GLEW::GLEW
    )
endif()

# Copy shader files to build directory
file(GLOB SHADER_FILES "*.glsl")
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_FILENAME ${SHADER_FILE} NAME)
    configure_file(${SHADER_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILENAME} COPYONLY)
endforeach()

message(STATUS "Building Rubik's Cube with sources: ${SOURCES}")
message(STATUS "Shader files: ${SHADER_FILES}")
message(STATUS "Including header files from: ${CMAKE_CURRENT_SOURCE_DIR}/include")