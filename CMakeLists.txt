cmake_minimum_required(VERSION 3.10)

option(DTV "Enable video output" OFF)

if (DTV)
    add_compile_definitions(ATG_ENGINE_SIM_VIDEO_CAPTURE)
endif (DTV)

# Enable group projects in folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmake")

project(engine-sim)

set(CMAKE_CXX_STANDARD 11)

add_executable(engine-sim WIN32
    # Source files
    src/main.cpp
    src/engine_sim_application.cpp
    src/geometry_generator.cpp
    src/crankshaft.cpp
    src/cylinder_bank.cpp
    src/connecting_rod.cpp
    src/piston.cpp
    src/part.cpp
    src/engine.cpp
    src/engine_simulator.cpp
    src/simulation_object.cpp
    src/piston_object.cpp
    src/connecting_rod_object.cpp

    # Include files
    include/delta.h
    include/dtv.h
    include/engine_sim_application.h
    include/geometry_generator.h
    include/crankshaft.h
    include/cylinder_bank.h
    include/connecting_rod.h
    include/piston.h
    include/part.h
    include/engine.h
    include/engine_simulator.h
    include/simulation_object.h
    include/piston_object.h
    include/connecting_rod_object.h
)

target_link_libraries(engine-sim
    simple-2d-constraint-solver
    delta-basic)

if (DTV)
    target_link_libraries(engine-sim
        direct-to-video)
endif (DTV)

target_include_directories(engine-sim
    PUBLIC dependencies/submodules)

add_subdirectory(dependencies)
