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
	
	${CMAKE_CURRENT_BINARY_DIR}/src/simgrid_units_main.c
)



if(PERL_EXECUTABLE)
	
	ADD_CUSTOM_COMMAND(
	  	OUTPUT	${TEST_UNITS}
			
  		DEPENDS	${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
			${TEST_CFILES}
  	
		COMMAND	${CMAKE_COMMAND} -E remove -f ${TEST_UNITS}
	
		COMMAND chmod +x ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
	
		COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl --root=src/ --outdir=${CMAKE_CURRENT_BINARY_DIR}/src/ ${TEST_CFILES}
	
		WORKING_DIRECTORY ${PROJECT_DIRECTORY}
	
	        COMMENT "Generating *_units files for testall..."
	)
	
	
else(PERL_EXECUTABLE)
	ADD_CUSTOM_COMMAND(
  	OUTPUT	${TEST_UNITS}
			
  	DEPENDS	${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
  		${TEST_CFILES}
			
	COMMAND	${CMAKE_COMMAND} message WARNING "Unit files need to be regenerated, but no Perl installed")
endif(PERL_EXECUTABLE)




### Ensure the build of testall

set_source_files_properties(${TEST_UNITS} PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/src/")
add_executable(testall ${TEST_UNITS})

### Add definitions for compile
if(NOT WIN32)
    target_link_libraries(testall gras m)
else(NOT WIN32)
    target_link_libraries(testall gras)
endif(NOT WIN32)
	
add_dependencies(testall $(TEST_UNITS))