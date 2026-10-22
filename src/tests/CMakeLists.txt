include_directories("${CMAKE_CURRENT_BINARY_DIR}/../")

add_executable(darktable-test-variables variables.c)
target_link_libraries(darktable-test-variables lib_darktable)

if(WIN32)
    # This tester sets up a darktable instance (of sorts). Hence it expects libraries at ../lib/darktable
    # Easiest way to comply with this on Windows: Put tester executable in same directory as darktable executable
    set_target_properties(darktable-test-variables PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${DARKTABLE_BINDIR}
    )
endif(WIN32)

add_subdirectory(unittests)

if(USE_LIBRAW AND NOT (DONT_USE_INTERNAL_LIBRAW AND libraw_FOUND))
    add_executable(darktable-test-nikon-z5ii nikon_z5ii.cc)
    target_link_libraries(darktable-test-nikon-z5ii PRIVATE libraw::libraw ${Glib_LIBRARIES})
    add_test(NAME imageio_nikon_z5ii COMMAND darktable-test-nikon-z5ii)
    set_tests_properties(imageio_nikon_z5ii PROPERTIES SKIP_RETURN_CODE 77)
endif()
