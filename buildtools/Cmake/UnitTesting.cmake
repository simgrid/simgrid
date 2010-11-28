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
	${PROJECT_DIRECTORY}/src/cunit_unit.c
	${PROJECT_DIRECTORY}/src/ex_unit.c
	${PROJECT_DIRECTORY}/src/dynar_unit.c
	${PROJECT_DIRECTORY}/src/dict_unit.c
	${PROJECT_DIRECTORY}/src/set_unit.c
	${PROJECT_DIRECTORY}/src/swag_unit.c
	${PROJECT_DIRECTORY}/src/xbt_str_unit.c
	${PROJECT_DIRECTORY}/src/xbt_strbuff_unit.c
	${PROJECT_DIRECTORY}/src/xbt_sha_unit.c
	${PROJECT_DIRECTORY}/src/config_unit.c
	${PROJECT_DIRECTORY}/src/xbt_synchro_unit.c
	
	${PROJECT_DIRECTORY}/src/simgrid_units_main.c
)



if(PERL_EXECUTABLE)
	
	ADD_CUSTOM_COMMAND(
	  	OUTPUT	${TEST_UNITS}
			
  		DEPENDS	${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
			${TEST_CFILES}
  	
		COMMAND	${CMAKE_COMMAND} -E remove -f ${TEST_UNITS}
	
		COMMAND chmod a=rwx ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
	
		COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl ${TEST_CFILES}
	
		WORKING_DIRECTORY ${PROJECT_DIRECTORY}/src
	
	        COMMENT "Generating *_units files for testall..."
	)
	
#      	add_custom_target(units_files	
#			DEPENDS ${TEST_UNITS}
#	)
	
else(PERL_EXECUTABLE)
	ADD_CUSTOM_COMMAND(
  	OUTPUT	${TEST_UNITS}
			
  	DEPENDS	${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
  		${TEST_CFILES}
			
	COMMAND	${CMAKE_COMMAND} message WARNING "Unit files need to be regenerated, but no Perl installed")
endif(PERL_EXECUTABLE)




### Ensure the build of testall

set_source_files_properties(${TEST_UNITS} PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/src/")
add_executable(testall ${TEST_UNITS})

### Add definitions for compile
if(NOT WIN32)
    target_link_libraries(testall gras m)
else(NOT WIN32)
    target_link_libraries(testall gras)
endif(NOT WIN32)
	
add_dependencies(testall $(TEST_UNITS))