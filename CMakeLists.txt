cmake_minimum_required(VERSION 3.10)

# Enable group projects in folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmake")

project(engine-sim)

set(CMAKE_CXX_STANDARD 11)

add_executable(engine-sim WIN32
    # Source files
    src/main.cpp
    src/engine_sim_application.cpp
    src/simple_pendulum_application.cpp
    src/geometry_generator.cpp
    src/matrix.cpp
    src/ode_solver.cpp
    src/euler_solver.cpp
    src/rk4_solver.cpp

    # Include files
    include/delta.h
    include/engine_sim_application.h
    include/simple_pendulum_application.h
    include/geometry_generator.h
    include/matrix.h
    include/ode_solver.h
    include/euler_solver.h
    include/rk4_solver.h
)

target_link_libraries(engine-sim
    delta-basic)

target_include_directories(engine-sim
    PUBLIC dependencies/submodules)

add_subdirectory(dependencies)
