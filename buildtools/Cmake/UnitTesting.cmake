if(PERL_EXECUTABLE)
	
	ADD_CUSTOM_COMMAND(
  	OUTPUT	${PROJECT_DIRECTORY}/src/cunit_unit.c
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
			
  	DEPENDS	${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
  			${PROJECT_DIRECTORY}/src/xbt/cunit.c
			${PROJECT_DIRECTORY}/src/xbt/ex.c
			${PROJECT_DIRECTORY}/src/xbt/dynar.c
			${PROJECT_DIRECTORY}/src/xbt/dict.c
			${PROJECT_DIRECTORY}/src/xbt/set.c
			${PROJECT_DIRECTORY}/src/xbt/swag.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_str.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_strbuff.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_sha.c
			${PROJECT_DIRECTORY}/src/xbt/config.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_synchro.c
  	
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/simgrid_units_main.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/cunit_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/ex_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/dynar_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/dict_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/set_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/swag_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt_str_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt_strbuff_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt_sha_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/config_unit.c
	COMMAND	${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/xbt_synchro_unit.c
	
	COMMAND chmod a=rwx ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
	
	#$(TEST_UNITS): $(TEST_CFILES)
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/cunit.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/ex.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/dynar.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/dict.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/set.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/swag.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/xbt_str.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/xbt_strbuff.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/xbt_sha.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/config.c
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/xbt_synchro.c
	
	#@builddir@/simgrid_units_main.c: $(TEST_UNITS)
	COMMAND ${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl xbt/cunit.c
	
	WORKING_DIRECTORY ${PROJECT_DIRECTORY}/src
	
	COMMENT "Generating *_units files for testall..."
	)
	
	add_custom_target(units_files
						DEPENDS ${PROJECT_DIRECTORY}/src/cunit_unit.c
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
						)
	
else(PERL_EXECUTABLE)
	ADD_CUSTOM_COMMAND(
  	OUTPUT	${PROJECT_DIRECTORY}/src/cunit_unit.c
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
			
  	DEPENDS	${PROJECT_DIRECTORY}/tools/sg_unit_extractor.pl
  			${PROJECT_DIRECTORY}/src/xbt/cunit.c
			${PROJECT_DIRECTORY}/src/xbt/ex.c
			${PROJECT_DIRECTORY}/src/xbt/dynar.c
			${PROJECT_DIRECTORY}/src/xbt/dict.c
			${PROJECT_DIRECTORY}/src/xbt/set.c
			${PROJECT_DIRECTORY}/src/xbt/swag.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_str.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_strbuff.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_sha.c
			${PROJECT_DIRECTORY}/src/xbt/config.c
			${PROJECT_DIRECTORY}/src/xbt/xbt_synchro.c
			
	COMMAND	${CMAKE_COMMAND} message "Unit files need to be regenerated, but no Perl installed")
endif(PERL_EXECUTABLE)


