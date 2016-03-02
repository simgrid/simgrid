# This file is in charge of the unit testing in SimGrid.
# See http://simgrid.gforge.inria.fr/simgrid/3.13/doc/inside_tests.html#inside_tests_add_units

# To register a file containing unit tests, simply add it to
# FILES_CONTAINING_UNITTESTS and have a pleasant day.

set(FILES_CONTAINING_UNITTESTS
  src/xbt/cunit.c
  src/xbt/ex.c
  src/xbt/dynar.c
  src/xbt/dict.c
  src/xbt/swag.c
  src/xbt/xbt_str.c
  src/xbt/xbt_strbuff.c
  src/xbt/config.c
)

if(HAVE_MC)
  set(FILES_CONTAINING_UNITTESTS ${FILES_CONTAINING_UNITTESTS}
      src/mc/PageStore.cpp
      src/mc/mc_snapshot.cpp
  )
endif()

####  Nothing to change below this line to add a new tested file
################################################################

foreach(file ${FILES_CONTAINING_UNITTESTS})
  get_filename_component(basename ${file} NAME_WE)
  get_filename_component(ext ${file} EXT)
  set(EXTRACTED_TEST_SOURCE_FILES ${EXTRACTED_TEST_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/src/${basename}_unit${ext})
endforeach()
  
set(EXTRACTED_TEST_SOURCE_FILES ${EXTRACTED_TEST_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/src/simgrid_units_main.c)

set_source_files_properties(${EXTRACTED_TEST_SOURCE_FILES} PROPERTIES GENERATED true)

ADD_CUSTOM_COMMAND(
  OUTPUT  ${EXTRACTED_TEST_SOURCE_FILES}
  DEPENDS ${CMAKE_HOME_DIRECTORY}/tools/sg_unit_extractor.pl ${FILES_CONTAINING_UNITTESTS}
  COMMENT "Generating *_units files for testall..."

  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
  COMMAND	    ${CMAKE_COMMAND} -E remove -f ${EXTRACTED_TEST_SOURCE_FILES}
  COMMAND           chmod +x ${CMAKE_HOME_DIRECTORY}/tools/sg_unit_extractor.pl
  COMMAND           ${CMAKE_HOME_DIRECTORY}/tools/sg_unit_extractor.pl --root=src/ --outdir=${CMAKE_CURRENT_BINARY_DIR}/src/ ${FILES_CONTAINING_UNITTESTS}
)

add_executable       (testall ${EXTRACTED_TEST_SOURCE_FILES})
target_link_libraries(testall simgrid)

