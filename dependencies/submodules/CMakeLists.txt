add_subdirectory(delta-studio)

set_property(TARGET delta-basic PROPERTY FOLDER "delta")
set_property(TARGET delta-basic-demo PROPERTY FOLDER "delta")
set_property(TARGET delta-core PROPERTY FOLDER "delta")
set_property(TARGET delta-physics PROPERTY FOLDER "delta")

add_subdirectory(simple-2d-constraint-solver)

set_property(TARGET simple-2d-constraint-solver PROPERTY FOLDER "scs")
set_property(TARGET simple-2d-constraint-solver-test PROPERTY FOLDER "scs")

if (DTV)
    add_subdirectory(direct-to-video)

    set_property(TARGET direct-to-video PROPERTY FOLDER "dtv")
    set_property(TARGET direct-to-video PROPERTY FOLDER "dtv")
endif (DTV)
