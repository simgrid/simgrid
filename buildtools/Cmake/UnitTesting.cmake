# To add a new tested file, simply add the original file in
# TEST_CFILES and generated file in TEST_UNITS. The rest is automatic.

set(TEST_CFILES
  src/xbt/cunit.c
  src/xbt/ex.c
  src/xbt/dynar.c
  src/xbt/dict.c
  src/xbt/set.c
  src/xbt/swag.c
  src/xbt/xbt_str.c
  src/xbt/xbt_strbuff.c
  src/xbt/xbt_sha.c
  src/xbt/config.c
  src/xbt/xbt_synchro.c
  src/xbt/parmap.c
  )
set(TEST_UNITS
  ${CMAKE_CURRENT_BINARY_DIR}/src/cunit_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/ex_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/dynar_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/dict_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/set_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/swag_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/xbt_str_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/xbt_strbuff_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/xbt_sha_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/config_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/xbt_synchro_unit.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/parmap_unit.c

  ${CMAKE_CURRENT_BINARY_DIR}/src/simgrid_units_main.c
  )

ADD_CUSTOM_COMMAND(
  OUTPUT	${TEST_UNITS}

  DEPENDS	${CMAKE_HOME_DIRECTORY}/tools/sg_unit_extractor.pl
  ${TEST_CFILES}

  COMMAND	${CMAKE_COMMAND} -E remove -f ${TEST_UNITS}

  COMMAND chmod +x ${CMAKE_HOME_DIRECTORY}/tools/sg_unit_extractor.pl

  COMMAND ${CMAKE_HOME_DIRECTORY}/tools/sg_unit_extractor.pl --root=src/ --outdir=${CMAKE_CURRENT_BINARY_DIR}/src/ ${TEST_CFILES}

  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}

  COMMENT "Generating *_units files for testall..."
  )

### Ensure the build of testall

set_source_files_properties(${TEST_UNITS} PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/src/")
add_executable(testall ${TEST_UNITS})

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(testall simgrid m)
else()
  target_link_libraries(testall simgrid)
endif()

add_dependencies(testall ${TEST_UNITS})