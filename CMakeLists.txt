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
    src/sle_solver.cpp
    src/gauss_seidel_sle_solver.cpp
    src/constraint.cpp
    src/fixed_position_constraint.cpp
    src/rigid_body.cpp
    src/rigid_body_system.cpp
    src/force_generator.cpp
    src/static_force_generator.cpp
    src/double_pendulum_application.cpp
    src/articulated_pendulum_application.cpp
    src/link_constraint.cpp
    src/simple_rotational_friction_generator.cpp
    src/line_constraint.cpp
    src/line_constraint_application.cpp

    # Include files
    include/delta.h
    include/engine_sim_application.h
    include/simple_pendulum_application.h
    include/geometry_generator.h
    include/matrix.h
    include/ode_solver.h
    include/euler_solver.h
    include/rk4_solver.h
    include/sle_solver.h
    include/gauss_seidel_sle_solver.h
    include/constraint.h
    include/fixed_position_constraint.h
    include/rigid_body.h
    include/rigid_body_system.h
    include/force_generator.h
    include/static_force_generator.h
    include/double_pendulum_application.h
    include/articulated_pendulum_application.h
    include/link_constraint.h
    include/simple_rotational_friction_generator.h
    include/line_constraint.h
    include/line_constraint_application.h
)

target_link_libraries(engine-sim
    delta-basic)

target_include_directories(engine-sim
    PUBLIC dependencies/submodules)

add_subdirectory(dependencies)
