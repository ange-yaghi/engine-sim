add_subdirectory(delta-studio)

set_property(TARGET delta-basic PROPERTY FOLDER "delta")
set_property(TARGET delta-basic-demo PROPERTY FOLDER "delta")
set_property(TARGET delta-core PROPERTY FOLDER "delta")
set_property(TARGET delta-physics PROPERTY FOLDER "delta")

add_subdirectory(simple-2d-constraint-solver)

set_property(TARGET simple-2d-constraint-solver PROPERTY FOLDER "scs")
set_property(TARGET simple-2d-constraint-solver-test PROPERTY FOLDER "scs")

add_subdirectory(csv-io)

set_property(TARGET csv-io PROPERTY FOLDER "csv-io")
set_property(TARGET csv-io-main PROPERTY FOLDER "csv-io")
set_property(TARGET csv-io-test PROPERTY FOLDER "csv-io")

if (DTV)
    add_subdirectory(direct-to-video)

    set_property(TARGET direct-to-video PROPERTY FOLDER "dtv")
    set_property(TARGET direct-to-video PROPERTY FOLDER "dtv")
endif (DTV)

add_subdirectory(piranha)

set_property(TARGET piranha PROPERTY FOLDER "piranha")
set_property(TARGET piranha_test PROPERTY FOLDER "piranha")
set_property(TARGET piranha_reference_compiler PROPERTY FOLDER "piranha")
