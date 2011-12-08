project(hpcc)

set( SRC
    hpcc.cpp
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HPCC_SOURCE_DIR}/system/jlib
    ${HPCC_SOURCE_DIR}/system/include
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/oss
    
)

add_executable( hpcc ${SRC} )
install ( TARGETS hpcc DESTINATION ${OSSDIR}/sbin )
target_link_libraries ( hpcc
    ssh2
    jlib
)
