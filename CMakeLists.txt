################################################################################
### metadata ###################################################################
################################################################################



################################################################################
### user config ################################################################
################################################################################



################################################################################
### sources ####################################################################
################################################################################

file(GLOB ssn_lib_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB ssn_lib_headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB ssn_dat_sources ${CMAKE_CURRENT_SOURCE_DIR}/ssn_data*.c*)
file(GLOB ssn_dat_headers ${CMAKE_CURRENT_SOURCE_DIR}/ssn_data*.h*)
list(REMOVE_ITEM ssn_lib_sources ${ssn_dat_sources})
list(REMOVE_ITEM ssn_lib_headers ${ssn_dat_headers})

################################################################################
### targets ####################################################################
################################################################################

llce_simulation(NAME ssn
                BASE_SOURCES ${ssn_lib_sources}
                DATA_SOURCES ${ssn_dat_sources})

################################################################################
### packaging ##################################################################
################################################################################



################################################################################
### sub-packaging ##############################################################
################################################################################


