cmake_minimum_required(VERSION 3.10)

option(DTV "Enable video output" OFF)
option(PIRANHA_ENABLED "Enable scripting input" OFF)

if (DTV)
    add_compile_definitions(ATG_ENGINE_SIM_VIDEO_CAPTURE)
endif (DTV)

if (PIRANHA_ENABLED)
    add_compile_definitions(ATG_ENGINE_PIRANHA_ENABLED)
endif (PIRANHA_ENABLED)

# Enable group projects in folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmake")

project(engine-sim)

set(CMAKE_CXX_STANDARD 11)

# ========================================================
# GTEST

include(FetchContent)
FetchContent_Declare(
    googletest
    URL
    https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set_property(TARGET gmock PROPERTY FOLDER "gtest")
set_property(TARGET gmock_main PROPERTY FOLDER "gtest")
set_property(TARGET gtest PROPERTY FOLDER "gtest")
set_property(TARGET gtest_main PROPERTY FOLDER "gtest")

# ========================================================

add_library(engine-sim STATIC
    # Source files
    src/crankshaft.cpp
    src/cylinder_bank.cpp
    src/connecting_rod.cpp
    src/piston.cpp
    src/part.cpp
    src/engine.cpp
    src/simulator.cpp
    src/combustion_chamber.cpp
    src/gas_system.cpp
    src/function.cpp
    src/camshaft.cpp
    src/cylinder_head.cpp
    src/dynamometer.cpp
    src/ignition_module.cpp
    src/utilities.cpp
    src/audio_buffer.cpp
    src/filter.cpp
    src/convolution_filter.cpp
    src/exhaust_system.cpp
    src/intake.cpp
    src/feedback_comb_filter.cpp
    src/synthesizer.cpp
    src/leveling_filter.cpp
    src/fuel.cpp
    src/gaussian_filter.cpp
    src/starter_motor.cpp
    src/transmission.cpp
    src/vehicle.cpp
    src/derivative_filter.cpp
    src/low_pass_filter.cpp
    src/vehicle_drag_constraint.cpp

    # Include files
    include/crankshaft.h
    include/cylinder_bank.h
    include/connecting_rod.h
    include/piston.h
    include/part.h
    include/engine.h
    include/simulator.h
    include/combustion_chamber.h
    include/units.h
    include/gas_system.h
    include/function.h
    include/camshaft.h
    include/cylinder_head.h
    include/dynamometer.h
    include/ignition_module.h
    include/utilities.h
    include/audio_buffer.h
    include/filter.h
    include/convolution_filter.h
    include/exhaust_system.h
    include/intake.h
    include/feedback_comb_filter.h
    include/synthesizer.h
    include/leveling_filter.h
    include/fuel.h
    include/gaussian_filter.h
    include/starter_motor.h
    include/transmission.h
    include/vehicle.h
    include/derivative_filter.h
    include/low_pass_filter.h
    include/vehicle_drag_constraint.h
)

target_link_libraries(engine-sim
    simple-2d-constraint-solver
    csv-io
    delta-basic)

target_include_directories(engine-sim
    PUBLIC dependencies/submodules)

if (PIRANHA_ENABLED)
    add_library(engine-sim-script-interpreter STATIC
        # Source files
        scripting/src/language_rules.cpp
        scripting/src/compiler.cpp

        # Include files
        scripting/include/compiler.h
        scripting/include/engine_sim.h
        scripting/include/language_rules.h
        scripting/include/piranha.h
    )

    target_include_directories(engine-sim-script-interpreter
        PUBLIC dependencies/submodules)

    target_link_libraries(engine-sim-script-interpreter
        csv-io
        piranha)
endif (PIRANHA_ENABLED)

add_executable(engine-sim-app WIN32
    # Source files
    src/main.cpp
    src/engine_sim_application.cpp
    src/geometry_generator.cpp
    src/simulation_object.cpp
    src/piston_object.cpp
    src/connecting_rod_object.cpp
    src/ui_element.cpp
    src/ui_manager.cpp
    src/cylinder_pressure_gauge.cpp
    src/ui_math.cpp
    src/gauge.cpp
    src/crankshaft_object.cpp
    src/cylinder_bank_object.cpp
    src/cylinder_head_object.cpp
    src/ui_button.cpp
    src/ui_utilities.cpp
    src/combustion_chamber_object.cpp
    src/oscilloscope.cpp
    src/shaders.cpp
    src/engine_view.cpp
    src/right_gauge_cluster.cpp
    src/cylinder_temperature_gauge.cpp
    src/labeled_gauge.cpp
    src/throttle_display.cpp
    src/afr_cluster.cpp
    src/fuel_cluster.cpp
    src/oscilloscope_cluster.cpp
    src/performance_cluster.cpp
    src/firing_order_display.cpp
    src/load_simulation_cluster.cpp
    src/mixer_cluster.cpp
    src/info_cluster.cpp

    # Include files
    include/delta.h
    include/dtv.h
    include/engine_sim_application.h
    include/geometry_generator.h
    include/simulation_object.h
    include/piston_object.h
    include/connecting_rod_object.h
    include/ui_element.h
    include/ui_manager.h
    include/cylinder_pressure_gauge.h
    include/ui_math.h
    include/units.h
    include/crankshaft_object.h
    include/cylinder_bank_object.h
    include/cylinder_head_object.h
    include/ui_button.h
    include/ui_utilities.h
    include/combustion_chamber_object.h
    include/oscilloscope.h
    include/shaders.h
    include/engine_view.h
    include/right_gauge_cluster.h
    include/cylinder_temperature_gauge.h
    include/labeled_gauge.h
    include/throttle_display.h
    include/afr_cluster.h
    include/fuel_cluster.h
    include/oscilloscope_cluster.h
    include/performance_cluster.h
    include/firing_order_display.h
    include/load_simulation_cluster.h
    include/mixer_cluster.h
    include/info_cluster.h
)

target_link_libraries(engine-sim-app
    engine-sim
)

if (DTV)
    target_link_libraries(engine-sim-app
        direct-to-video)
endif (DTV)

if (PIRANHA_ENABLED)
    target_link_libraries(engine-sim-app
        engine-sim-script-interpreter)
endif (PIRANHA_ENABLED)

target_include_directories(engine-sim-app
    PUBLIC dependencies/submodules)

add_subdirectory(dependencies)

# GTEST

enable_testing()

add_executable(engine-sim-test
    # Source files
    test/gas_system_tests.cpp
    test/function_test.cpp
    test/synthesizer_tests.cpp
)

target_link_libraries(engine-sim-test
    gtest_main
    engine-sim
)

include(GoogleTest)
gtest_discover_tests(engine-sim-test)
