### Make supernovae files and libs

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
set(temp_src_smpi "")
set(temp_src_simgrid "")
set(temp_src_gras "")

if(enable_smpi)
	foreach(file ${SMPI_SRC})
		set(temp_src_smpi "${temp_src_smpi} ${file}")
	endforeach(file ${SMPI_SRC})
endif(enable_smpi)

foreach(file ${simgrid_sources})
	set(en_plus yes)
	foreach(file_delete ${simgrid_fragile_sources})
		if(file_delete MATCHES "${file}")
			set(en_plus no)
			#message("${file}")
		endif(file_delete MATCHES "${file}")
	endforeach (file_delete ${simgrid_fragile_sources})
	if(en_plus)
		if(file MATCHES "src/xbt/log.c")
			set(file "xbt/log.c")
		endif(file MATCHES "src/xbt/log.c")
		set(temp_src_simgrid "${temp_src_simgrid} ${file}")
	endif(en_plus)
endforeach(file ${simgrid_sources})

foreach(file ${gras_sources})
	set(en_plus yes)
	foreach(file_delete ${gras_fragile_sources})
		if(file_delete MATCHES "${file}")
			set(en_plus no)
			#message("${file}")
		endif(file_delete MATCHES "${file}")
	endforeach (file_delete ${gras_fragile_sources})
	if(en_plus)
		if(file MATCHES "src/xbt/log.c")
			set(file "xbt/log.c")
		endif(file MATCHES "src/xbt/log.c")
		set(temp_src_gras "${temp_src_gras} ${file}")
	endif(en_plus)
endforeach(file ${gras_sources})

exec_program("${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/supernovae_sg.c ${PROJECT_DIRECTORY}/src/supernovae_gras.c ${PROJECT_DIRECTORY}/src/supernovae_smpi.c" OUTPUT_VARIABLE "SUPER_OK")
exec_program("chmod a=rwx ${PROJECT_DIRECTORY}/src/mk_supernovae.sh" OUTPUT_VARIABLE "SUPER_OK")
exec_program("${PROJECT_DIRECTORY}/src/mk_supernovae.sh ${PROJECT_DIRECTORY}/src/supernovae_sg.c   ${temp_src_simgrid}" "${PROJECT_DIRECTORY}" OUTPUT_VARIABLE "SUPER_OK")
exec_program("${PROJECT_DIRECTORY}/src/mk_supernovae.sh ${PROJECT_DIRECTORY}/src/supernovae_gras.c ${temp_src_gras}" "${PROJECT_DIRECTORY}"	OUTPUT_VARIABLE "SUPER_OK")
if(enable_smpi)
	exec_program("${PROJECT_DIRECTORY}/src/mk_supernovae.sh ${PROJECT_DIRECTORY}/src/supernovae_smpi.c ${temp_src_smpi}" "${PROJECT_DIRECTORY}"	OUTPUT_VARIABLE "SUPER_OK")
endif(enable_smpi)

add_library(simgrid 	SHARED 	${PROJECT_DIRECTORY}/src/supernovae_sg.c ${simgrid_fragile_sources})
add_library(gras 	SHARED 	${PROJECT_DIRECTORY}/src/supernovae_gras.c ${gras_fragile_sources})
if(enable_smpi)
	add_library(smpi 	SHARED 	${PROJECT_DIRECTORY}/src/supernovae_smpi.c)
endif(enable_smpi)
