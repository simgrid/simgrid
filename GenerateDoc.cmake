#### Generate the html documentation

if(BIBTEX2HTML)
	set(BIBTEX2HTML_PATH ${BIBTEX2HTML})
else(BIBTEX2HTML)
	find_path(BIBTEX2HTML_PATH	NAMES bibtex2html	PATHS NO_DEFAULT_PATHS)
endif(BIBTEX2HTML)

find_path(FIG2DEV_PATH	NAMES fig2dev	PATHS NO_DEFAULT_PATHS)
find_path(DOXYGEN_PATH	NAMES doxygen	PATHS NO_DEFAULT_PATHS)

### Check whether the bibtex2html that we found is the one that Arnaud requires
exec_program("${BIBTEX2HTML_PATH}/bibtex2html -version" OUTPUT_VARIABLE OUTPUT_BIBTEX2HTML_VERSION)
STRING(REPLACE "[-bibtex]" "" OUTPUT_BIBTEX2HTML_VERSION_2 ${OUTPUT_BIBTEX2HTML_VERSION})

if(${OUTPUT_BIBTEX2HTML_VERSION_2} STREQUAL ${OUTPUT_BIBTEX2HTML_VERSION}) # wrong version
	SET(GOOD_BIBTEX2HTML_VERSION 0)
else(${OUTPUT_BIBTEX2HTML_VERSION_2} STREQUAL ${OUTPUT_BIBTEX2HTML_VERSION}) # good version
	SET(GOOD_BIBTEX2HTML_VERSION 1)
endif(${OUTPUT_BIBTEX2HTML_VERSION_2} STREQUAL ${OUTPUT_BIBTEX2HTML_VERSION})

if(DOXYGEN_PATH AND FIG2DEV_PATH AND BIBTEX2HTML_PATH AND GOOD_BIBTEX2HTML_VERSION)
	
	configure_file(${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in ${CMAKE_HOME_DIRECTORY}/doc/Doxyfile @ONLY)
	configure_file(${CMAKE_HOME_DIRECTORY}/doc/footer.html.in ${CMAKE_HOME_DIRECTORY}/doc/footer.html @ONLY)		
	
	ADD_CUSTOM_TARGET(documentation
		COMMENT "Generating the SimGrid documentation..."
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_HOME_DIRECTORY}/doc/html
	    COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_HOME_DIRECTORY}/doc/html
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc
	)
	
	ADD_CUSTOM_COMMAND(TARGET documentation
	    COMMAND ${CMAKE_COMMAND} -E echo "XX Doxygen pass"
		COMMAND ${DOXYGEN_PATH}/doxygen Doxyfile
		
        COMMAND ${CMAKE_COMMAND} -E echo "XX Javadoc pass"
        COMMAND javadoc -d ${CMAKE_HOME_DIRECTORY}/doc/html/javadoc/ ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/*.java
		
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/
	)
	
else(DOXYGEN_PATH AND FIG2DEV_PATH AND BIBTEX2HTML_PATH AND GOOD_BIBTEX2HTML_VERSION)

	ADD_CUSTOM_TARGET(documentation
			COMMENT "Generating the SimGrid documentation..."
			)

	if(NOT GOOD_BIBTEX2HTML_VERSION) # wrong version
		ADD_CUSTOM_COMMAND(TARGET documentation
			COMMAND ${CMAKE_COMMAND} -E echo "This is not the good bibtex2html program !!!"
			COMMAND ${CMAKE_COMMAND} -E echo  "You can download it from:"
			COMMAND ${CMAKE_COMMAND} -E echo  "  ftp://ftp-sop.inria.fr/epidaure/Softs/bibtex2html/bibtex2html-1.02.tar.gz"
			COMMAND ${CMAKE_COMMAND} -E echo  "There is also an unofficial Debian/Ubuntu package, see:"
		        COMMAND ${CMAKE_COMMAND} -E echo  "  http://www.loria.fr/~lnussbau/bibtex2html/README"
			)
	endif(NOT GOOD_BIBTEX2HTML_VERSION)

	ADD_CUSTOM_COMMAND(TARGET documentation
			COMMAND ${CMAKE_COMMAND} -E echo "DOXYGEN_PATH 		= ${DOXYGEN_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "FIG2DEV_PATH 		= ${FIG2DEV_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "BIBTEX2HTML_PATH 	= ${BIBTEX2HTML_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "IN ORDER TO GENERATE THE DOCUMENTATION YOU NEED ALL TOOLS !!!"
			COMMAND ${CMAKE_COMMAND} -E echo "FAIL TO MAKE SIMGRID DOCUMENTATION see previous messages for details ..."
			COMMAND false
			)

		
endif(DOXYGEN_PATH AND FIG2DEV_PATH AND BIBTEX2HTML_PATH AND GOOD_BIBTEX2HTML_VERSION)
