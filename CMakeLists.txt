cmake_minimum_required(VERSION 3.10)

# Enable group projects in folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmake")

project(delta-template)

set(CMAKE_CXX_STANDARD 11)

add_executable(delta-template WIN32
    # Source files
    src/main.cpp
    src/template_application.cpp

    # Include files
    include/delta.h
    include/template_application.h
)

target_link_libraries(delta-template
    delta-basic)

target_include_directories(delta-template
    PUBLIC dependencies/submodules)

add_subdirectory(dependencies)
