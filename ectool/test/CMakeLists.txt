set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

add_executable( testectool testectool.cpp )

# TO_DO: nConnectRet failing causing all tests to fail
add_dependencies( unit_tests testectool )

target_link_libraries( testectool PRIVATE
    DBus
    Boost::date_time
    Boost::unit_test_framework
    cam
    dz
    evbus
    evbusutil
)

add_test( 
    NAME testectool
    COMMAND testectool
    CONFIGURATIONS failing # checkin nightly ectool
)
