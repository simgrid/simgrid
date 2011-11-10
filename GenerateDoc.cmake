#### Generate the html documentation
find_path(DOXYGEN_PATH	NAMES doxygen	PATHS NO_DEFAULT_PATHS)
find_path(JAVADOC_PATH  NAMES javadoc   PATHS NO_DEFAULT_PATHS)

if(DOXYGEN_PATH AND JAVADOC_PATH)
	
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
        COMMAND ${JAVADOC_PATH}/javadoc -d ${CMAKE_HOME_DIRECTORY}/doc/html/javadoc/ ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/*.java
		
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/
	)
	
else(DOXYGEN_PATH AND JAVADOC_PATH)
	ADD_CUSTOM_TARGET(documentation
			COMMENT "Generating the SimGrid documentation..."
			)

	ADD_CUSTOM_COMMAND(TARGET documentation
			COMMAND ${CMAKE_COMMAND} -E echo "DOXYGEN_PATH 		= ${DOXYGEN_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "JAVADOC_PATH      = ${JAVADOC_PATH}"
			COMMAND ${CMAKE_COMMAND} -E echo "FAIL TO MAKE SIMGRID DOCUMENTATION see previous messages for details ..."
			COMMAND false
			)	
endif(DOXYGEN_PATH AND JAVADOC_PATH)
