################################################################################
### metadata ###################################################################
################################################################################



################################################################################
### user config ################################################################
################################################################################



################################################################################
### sources ####################################################################
################################################################################

file(GLOB         ssn_lib_sources     ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB         ssn_lib_headers     ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB         ssn_dat_sources     ${CMAKE_CURRENT_SOURCE_DIR}/ssn_data*.c*)
file(GLOB         ssn_dat_headers     ${CMAKE_CURRENT_SOURCE_DIR}/ssn_data*.h*)
list(REMOVE_ITEM  ssn_lib_sources     ${ssn_dat_sources})
list(REMOVE_ITEM  ssn_lib_headers     ${ssn_dat_headers})

################################################################################
### targets ####################################################################
################################################################################

if(BUILD_SHARED_LIBS)
    set(ssn_lib_type MODULE)
endif()

add_library(ssndata ${ssn_lib_type} ${ssn_dat_sources})
add_library(ssn ${ssn_lib_type} ${ssn_lib_sources})
foreach(ssn_target ssndata ssn)
    target_include_directories(${ssn_target} PRIVATE ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(${ssn_target} PUBLIC llceutil ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})
endforeach()

if(NOT BUILD_SHARED_LIBS)
    target_link_libraries(ssn PRIVATE ssndata)
endif()

################################################################################
### packaging ##################################################################
################################################################################

# TODO(JRC): The solution below works when running CMake in serial because the
# "install.lock" file will necessarily be written before any changes are made
# to the project's install targets and be removed afterwards based on CMake's
# install script generation rules. It isn't clear whether or not these rules
# extend to parallel builds (though they probably do at least for Makefile outputs).

install(CODE "file(WRITE ${CMAKE_INSTALL_PREFIX}/install.lock \"\")")
install(TARGETS ssndata DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS ssn DESTINATION ${CMAKE_INSTALL_PREFIX})
install(CODE "file(REMOVE ${CMAKE_INSTALL_PREFIX}/install.lock)")

################################################################################
### sub-packaging ##############################################################
################################################################################


