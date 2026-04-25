cmake_minimum_required(VERSION 3.18)
project(platformer)

set(CMAKE_CXX_STANDARD 17)

if (APPLE)
    # Mac specific configuration
    message(STATUS "Configuring for macOS")

    # raylib
    find_package(raylib QUIET)
    if (NOT raylib_FOUND)
        include(FetchContent)
        FetchContent_Declare(
                raylib
                GIT_REPOSITORY https://github.com/raysan5/raylib.git
                GIT_TAG 4.2.0
        )
        FetchContent_MakeAvailable(raylib)
    endif()

    # raylib-cpp
    find_package(raylib_cpp QUIET)
    if (NOT raylib_cpp_FOUND)
        include(FetchContent)
        FetchContent_Declare(
                raylib_cpp
                GIT_REPOSITORY https://github.com/RobLoach/raylib-cpp.git
                GIT_TAG v4.2.6
        )
        FetchContent_MakeAvailable(raylib_cpp)
    endif()

    # Define the source files
    set(SOURCES
            main.cpp
    )

    add_executable(${PROJECT_NAME} ${SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)

    target_link_libraries(${PROJECT_NAME} PUBLIC raylib raylib_cpp)

    # Copy resources to the build directory
    file(COPY ${RESOURCE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

elseif (UNIX AND NOT APPLE)
    # Linux specific configuration
    message(STATUS "Configuring for Linux")

    # Add /usr/local/include to the include directories
    include_directories(/usr/local/include)

    # Define the source files
    set(SOURCES
            main.cpp
    )

    add_executable(${PROJECT_NAME} ${SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)

    # Link raylib library with the executable
    target_link_libraries(${PROJECT_NAME} /usr/local/lib/libraylib.a)

    # Copy resources to the build directory
    file(COPY ${RESOURCE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Define the output directory relative to the current build directory
set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/../../../)

# Ensure the output directory exists
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Add a post-build command to move the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}> ${OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>
        COMMENT "Moving ${PROJECT_NAME} to ${OUTPUT_DIR}"
)
