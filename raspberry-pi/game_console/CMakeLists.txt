cmake_minimum_required(VERSION 3.18)
project(game_console)

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
            src/systemStats.h
            src/systemStats.cpp
            src/CustomColors.h
            src/ui/Background.cpp
            src/ui/MenuButton.cpp
            src/ui/Background.h
            src/ui/MenuButton.h
            resources/satoshi.otf
            resources/AudioIcon.png
            resources/BackArrow.png
            resources/BrightnessIcon.png
            resources/GamesIcon.png
            resources/SettingsIcon.png
            resources/DeveloperIcon.png
            resources/BatteryIcon.png
            src/json.hpp
            src/ui/MoreSettings.cpp
            src/ui/MoreSettings.h
            src/ui/Row.cpp   # Ensure Row.cpp is included
            src/ui/Row.h
            src/gameLoader.cpp
            src/gameLoader.h
    )

    add_executable(${PROJECT_NAME} ${SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)

    target_link_libraries(${PROJECT_NAME} PUBLIC raylib raylib_cpp)

    # Define the directory where resources are stored
    set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

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
            src/systemStats.h
            src/systemStats.cpp
            src/CustomColors.h
            src/ui/Background.cpp
            src/ui/MenuButton.cpp
            src/ui/Background.h
            src/ui/MenuButton.h
            src/json.hpp
            src/ui/MoreSettings.cpp
            src/ui/MoreSettings.h
            src/ui/Row.cpp   # Ensure Row.cpp is included
            src/ui/Row.h
            src/gameLoader.cpp
            src/gameLoader.h
    )

    add_executable(${PROJECT_NAME} ${SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)

    # Link raylib library with the executable
    target_link_libraries(${PROJECT_NAME} /usr/local/lib/libraylib.a)

    # Define the directory where resources are stored
    set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

    # Copy resources to the build directory
    file(COPY ${RESOURCE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
