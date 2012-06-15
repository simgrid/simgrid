### Make supernovae files and libs

#############################################################################
### Add here every files that should not be supernovaed (generated files) ###
#############################################################################
set(simgrid_fragile_sources 
  src/simdag/sd_daxloader.c
  src/surf/surfxml_parse.c
  src/xbt/automaton/automaton_create.c
  src/xbt/datadesc/ddt_parse.yy.c
  src/xbt/graphxml_parse.c
  src/xbt/mmalloc/mm.c
  ${GTNETS_USED}
)
set(gras_fragile_sources
  src/xbt/automaton/automaton_create.c
  src/xbt/datadesc/ddt_parse.yy.c
  src/xbt/graphxml_parse.c
  src/xbt/mmalloc/mm.c
)

#####################################################
### END OF CONFIGURATION, NO NEED TO READ FURTHER ###
#####################################################

### Rebuild the supernovae source files
if (enable_supernovae) # I need supernovae

	# supernovae files are generated. I promise
	set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c
				      PROPERTIES GENERATED true)
	set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c
				      PROPERTIES GENERATED true)
	set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c
				      PROPERTIES GENERATED true)

	ADD_CUSTOM_COMMAND(
		OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c
		DEPENDS  ${CMAKE_HOME_DIRECTORY}/src/mk_supernovae.pl ${simgrid_sources}
		COMMAND  perl ${CMAKE_HOME_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c '--fragile=${simgrid_fragile_sources}' '${simgrid_sources}'
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
		COMMENT "Generating supernovae_sg.c"
	)

	ADD_CUSTOM_COMMAND(
		OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c
		DEPENDS  ${CMAKE_HOME_DIRECTORY}/src/mk_supernovae.pl ${gras_sources}
		COMMAND  perl ${CMAKE_HOME_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c '--fragile=${gras_fragile_sources}'    '${gras_sources}'
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
		COMMENT "Generating supernovae_gras.c"
	)

	ADD_CUSTOM_COMMAND(
		OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c
		DEPENDS  ${CMAKE_HOME_DIRECTORY}/src/mk_supernovae.pl ${SMPI_SRC}
		COMMAND  perl ${CMAKE_HOME_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c  '${SMPI_SRC}'
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
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
				
endif(enable_supernovae) # I need supernovae
