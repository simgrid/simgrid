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

set_source_files_properties(${PROJECT_DIRECTORY}/src/supernovae_sg.c;${PROJECT_DIRECTORY}/src/supernovae_gras.c;${PROJECT_DIRECTORY}/src/supernovae_smpi.c
			      PROPERTIES GENERATED true)

exec_program("${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/supernovae_sg.c
                                            ${PROJECT_DIRECTORY}/src/supernovae_gras.c 
					    ${PROJECT_DIRECTORY}/src/supernovae_smpi.c"
	OUTPUT_VARIABLE "SUPER_OK")

exec_program("chmod +x ${PROJECT_DIRECTORY}/src/mk_supernovae.pl" OUTPUT_VARIABLE "SUPER_OK")

ADD_CUSTOM_COMMAND(
	OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c
	DEPENDS  ${PROJECT_DIRECTORY}/src/mk_supernovae.pl
	COMMAND  perl ${PROJECT_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c '--fragile=${simgrid_fragile_sources}' '${simgrid_sources}'
	WORKING_DIRECTORY ${PROJECT_DIRECTORY}
	COMMENT "Generating supernovae_sg.c"
)

ADD_CUSTOM_COMMAND(
	OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c
	DEPENDS  ${PROJECT_DIRECTORY}/src/mk_supernovae.pl
	COMMAND  perl ${PROJECT_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c '--fragile=${gras_fragile_sources}'    '${gras_sources}'
	WORKING_DIRECTORY ${PROJECT_DIRECTORY}
	COMMENT "Generating supernovae_gras.c"
)

ADD_CUSTOM_COMMAND(
	OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c
	DEPENDS  ${PROJECT_DIRECTORY}/src/mk_supernovae.pl
	COMMAND  perl ${PROJECT_DIRECTORY}/src/mk_supernovae.pl --out=${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c  '${SMPI_SRC}'
	WORKING_DIRECTORY ${PROJECT_DIRECTORY}
	COMMENT "Generating supernovae_smpi.c"
)

### Make sure that the libs are built from the supernovae sources
	add_library(simgrid 	SHARED 	${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c ${simgrid_fragile_sources})
	add_dependencies(simgrid ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c)
	
	if(enable_lib_static)
		add_library(simgrid_static STATIC ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c ${simgrid_fragile_sources})
		add_dependencies(simgrid_static ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c)
	endif(enable_lib_static)
	
	add_library(gras 	SHARED 	${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c ${gras_fragile_sources})
	add_dependencies(gras ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c)

	if(enable_smpi)
		add_library(smpi 	SHARED 	${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
		add_dependencies(smpi ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
			if(enable_lib_static)
				add_library(smpi_static STATIC ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)	
				add_dependencies(smpi_static ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
			endif(enable_lib_static)
	endif(enable_smpi)
