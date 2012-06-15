#### Generate the html documentation

find_path(FIG2DEV_PATH	NAMES fig2dev	PATHS NO_DEFAULT_PATHS)
find_path(DOXYGEN_PATH	NAMES doxygen	PATHS NO_DEFAULT_PATHS)


file(GLOB_RECURSE source_doxygen
	"${CMAKE_HOME_DIRECTORY}/tools/gras/*.[chl]"
	"${CMAKE_HOME_DIRECTORY}/src/*.[chl]"
	"${CMAKE_HOME_DIRECTORY}/include/*.[chl]"
)


if(DOXYGEN_PATH AND FIG2DEV_PATH)

	#DOC_SOURCE=doc/*.doc, defined in DefinePackage
	set(DOCSSOURCES "${source_doxygen}\n${DOC_SOURCE}")
	string(REPLACE "\n" ";" DOCSSOURCES ${DOCSSOURCES})


	set(DOC_PNGS 
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_2011.png
		${CMAKE_HOME_DIRECTORY}/doc/webcruft/simgrid_logo_small.png
	)
	
	configure_file(${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/Doxyfile @ONLY)
	configure_file(${CMAKE_HOME_DIRECTORY}/doc/footer.html.in ${CMAKE_HOME_DIRECTORY}/doc/footer.html @ONLY)		
	
	ADD_CUSTOM_TARGET(ref_guide
		COMMENT "Generating the SimGrid ref guide..."
		DEPENDS ${DOC_SOURCES} ${DOC_FIGS} ${source_doxygen}
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/html
	    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/ref_guide/html
		COMMAND ${FIG2DEV_PATH}/fig2dev -Lmap ${CMAKE_HOME_DIRECTORY}/doc/fig/simgrid_modules.fig | perl -pe 's/imagemap/simgrid_modules/g'| perl -pe 's/<IMG/<IMG style=border:0px/g' > ${CMAKE_HOME_DIRECTORY}/doc/simgrid_modules.map
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
	)
		
	ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_HOME_DIRECTORY}/doc/logcategories.doc
		DEPENDS ${source_doxygen}
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/logcategories.doc
		COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/xbt_log_extract_hierarchy.pl > ${CMAKE_HOME_DIRECTORY}/doc/logcategories.doc
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
	)

	foreach(file ${DOC_FIGS})
		string(REPLACE ".fig" ".png" tmp_file ${file})
		string(REPLACE "${CMAKE_HOME_DIRECTORY}/doc/fig/" "${CMAKE_HOME_DIRECTORY}/doc/html/" tmp_file ${tmp_file})
		ADD_CUSTOM_COMMAND(TARGET ref_guide
			COMMAND ${FIG2DEV_PATH}/fig2dev -Lpng -S 4 ${file} ${tmp_file}
		)
	endforeach(file ${DOC_FIGS})
	
	foreach(file ${DOC_PNGS})
		ADD_CUSTOM_COMMAND(TARGET ref_guide
			COMMAND ${CMAKE_COMMAND} -E copy ${file} ${CMAKE_HOME_DIRECTORY}/doc/html/
		)
	endforeach(file ${DOC_PNGS})

	ADD_CUSTOM_COMMAND(TARGET ref_guide
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/doc/simgrid.css                          ${CMAKE_HOME_DIRECTORY}/doc/html/
	)
	
	ADD_CUSTOM_COMMAND(TARGET ref_guide
	    COMMAND ${CMAKE_COMMAND} -E echo "XX First Doxygen pass"
		COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
		COMMAND ${CMAKE_HOME_DIRECTORY}/tools/doxygen/index_create.pl simgrid.tag index-API.doc
		
		COMMAND ${CMAKE_COMMAND} -E echo "XX Second Doxygen pass"
		COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
		
		COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/html/dir*
		
		COMMAND ${CMAKE_COMMAND} -E echo "XX Create shortcuts pages"
		COMMAND ${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__GRAS__API.html'>\" > ${CMAKE_HOME_DIRECTORY}/doc/html/gras.html
		COMMAND ${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__GRAS__API.html'>Grid Reality And Simulation.</a></h2></center></html>\" >> ${CMAKE_HOME_DIRECTORY}/doc/html/gras.html
		
		COMMAND ${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__AMOK__API.html'>\" > ${CMAKE_HOME_DIRECTORY}/doc/html/amok.html
		COMMAND ${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__AMOK__API.html'>Advanced Metacomputing Overlay Kit.</a></h2></center></html>\" >> ${CMAKE_HOME_DIRECTORY}/doc/html/amok.html
	
		COMMAND ${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__MSG__API.html'>\" > ${CMAKE_HOME_DIRECTORY}/doc/html/msg.html
		COMMAND ${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__MSG__API.html'>Meta SimGrid.</a></h2></center></html>\" >> ${CMAKE_HOME_DIRECTORY}/doc/html/msg.html
	
		COMMAND ${CMAKE_COMMAND} -E echo \"<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__SD__API.html'>\" > ${CMAKE_HOME_DIRECTORY}/doc/html/simdag.html
		COMMAND ${CMAKE_COMMAND} -E echo \"<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__SD__API.html'>DAG Simulator.</a></h2></center></html>\" >> ${CMAKE_HOME_DIRECTORY}/doc/html/simdag.html
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/
	)
	
else(DOXYGEN_PATH AND FIG2DEV_PATH)

	ADD_CUSTOM_TARGET(ref_guide
			COMMENT "Generating the SimGrid documentation..."
			)

	ADD_CUSTOM_COMMAND(TARGET ref_guide
			COMMAND ${CMAKE_COMMAND} -E echo "DOXYGEN_PATH 		= ${DOXYGEN_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "FIG2DEV_PATH 		= ${FIG2DEV_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "IN ORDER TO GENERATE THE DOCUMENTATION YOU NEED ALL TOOLS !!!"
			COMMAND ${CMAKE_COMMAND} -E echo "FAIL TO MAKE SIMGRID DOCUMENTATION see previous messages for details ..."
			COMMAND false
			)

		
endif(DOXYGEN_PATH AND FIG2DEV_PATH)

##############################################################################"


ADD_CUSTOM_TARGET(ref_guide_pdf
    COMMAND ${CMAKE_COMMAND} -E echo "XX First pass simgrid_documentation.pdf"
    COMMAND make clean
    COMMAND make pdf || true
    COMMAND ${CMAKE_COMMAND} -E echo "XX Second pass simgrid_documentation.pdf"
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_HOME_DIRECTORY}/doc/latex/refman.pdf
    COMMAND make pdf || true
    COMMAND ${CMAKE_COMMAND} -E echo "XX Write Simgrid_documentation.pdf"
    COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_HOME_DIRECTORY}/doc/latex/refman.pdf ${CMAKE_HOME_DIRECTORY}/doc/latex/simgrid_documentation.pdf
  
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/latex/
)
add_dependencies(pdf ref_guide)
