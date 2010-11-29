### Make supernovae files and libs

#############################################################################
### Add here every files that should not be supernovaed (generated files) ###
#############################################################################
set(simgrid_fragile_sources 
	src/gras/DataDesc/ddt_parse.yy.c
	src/surf/surfxml_parse.c
	src/xbt/graphxml_parse.c
	src/simdag/sd_daxloader.c
)
set(gras_fragile_sources
	src/gras/DataDesc/ddt_parse.yy.c
	src/xbt/graphxml_parse.c
)

#####################################################
### END OF CONFIGURATION, NO NEED TO READ FURTHER ###
#####################################################

### Rebuild the supernovae source files
if (PERL_EXECUTABLE AND enable_supernovae) # I need supernovae and can use it

	# supernovae files are generated. I promise
	set_source_files_properties(${PROJECT_DIRECTORY}/src/supernovae_sg.c
				      PROPERTIES GENERATED true)
	set_source_files_properties(${PROJECT_DIRECTORY}/src/supernovae_gras.c
				      PROPERTIES GENERATED true)
	set_source_files_properties(${PROJECT_DIRECTORY}/src/supernovae_smpi.c
				      PROPERTIES GENERATED true)

	ADD_CUSTOM_COMMAND(
		OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c
		DEPENDS  ${PROJECT_DIRECTORY}/src/mk_supernovae.pl ${simgrid_sources}
		COMMAND  perl ${PROJECT_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c '--fragile=${simgrid_fragile_sources}' '${simgrid_sources}'
		WORKING_DIRECTORY ${PROJECT_DIRECTORY}
		COMMENT "Generating supernovae_sg.c"
	)

	ADD_CUSTOM_COMMAND(
		OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c
		DEPENDS  ${PROJECT_DIRECTORY}/src/mk_supernovae.pl ${gras_sources}
		COMMAND  perl ${PROJECT_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c '--fragile=${gras_fragile_sources}'    '${gras_sources}'
		WORKING_DIRECTORY ${PROJECT_DIRECTORY}
		COMMENT "Generating supernovae_gras.c"
	)

	ADD_CUSTOM_COMMAND(
		OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c
		DEPENDS  ${PROJECT_DIRECTORY}/src/mk_supernovae.pl ${SMPI_SRC}
		COMMAND  perl ${PROJECT_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c  '${SMPI_SRC}'
		WORKING_DIRECTORY ${PROJECT_DIRECTORY}
		COMMENT "Generating supernovae_smpi.c"
	)

	### Change the content of the libraries so that it contains only supernovae+fragiles
	set(simgrid_sources 
		${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c
		${simgrid_fragile_sources})

	set(gras_sources
		${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c 
		${gras_fragile_sources})		

	set(SMPI_SRC 
		${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
else(PERL_EXECUTABLE AND enable_supernovae) # I need supernovae and can use it
	if (enable_supernovae)
		message(You need Perl to activate supernovae)
		set(enable_supernovae 0)
	endif(enable_supernovae)
endif(PERL_EXECUTABLE AND enable_supernovae) # I need supernovae and can use it
