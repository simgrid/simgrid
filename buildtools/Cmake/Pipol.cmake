# On a Pipol system, set:
# PIPOL_USER

#SET(PIPOL_USER $ENV{PIPOL_USER})

# ssh/rsync mandatory 
IF(pipol_user)

	FIND_PROGRAM(HAVE_SSH ssh)
	FIND_PROGRAM(HAVE_RSYNC rsync)

    MESSAGE(STATUS "Pipol user is ${pipol_user}")
    IF(HAVE_SSH)
      # get pipol systems
      EXECUTE_PROCESS(COMMAND 
        ssh ${pipol_user}@pipol.inria.fr pipol-sub --query=systems 
        OUTPUT_VARIABLE PIPOL_SYSTEMS OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF(HAVE_SSH)
  
	ADD_CUSTOM_TARGET(pipol_test_list_images
		COMMENT "Available images for pipol tests (cmake + make + make test) : "
		)
		
	ADD_CUSTOM_TARGET(pipol_experimental_list_images
		COMMENT "Available images for ctest (ctest -D Experimental) : "
		)
		
	ADD_CUSTOM_TARGET(pipol_kvm_deploy
		COMMENT "Deploy all kvm images on pipol (ctest -D Experimental) : "
		)
	ADD_CUSTOM_COMMAND(TARGET pipol_kvm_deploy
		POST_BUILD
		COMMENT "See results on http://cdash.inria.fr/CDash/index.php?project=Simgrid"
		)
  
    IF(HAVE_RSYNC)
	      
	      MACRO(PIPOL_TARGET
	          SYSTEM_PATTERN)
				  STRING(REPLACE ".dd.gz" "" SYSTEM_TARGET ${SYSTEM_PATTERN})
				
			      ADD_CUSTOM_TARGET(
			        ${SYSTEM_TARGET}
			        COMMENT "PIPOL Build : ${SYSTEM_PATTERN}"
			        COMMAND rsync ${pipol_user}@pipol.inria.fr:/usr/local/bin/pipol-sub .
			        COMMAND ./pipol-sub --pipol-user=${pipol_user} ${SYSTEM_PATTERN} 02:00 --reconnect --group --keep --verbose=1 --export=${CMAKE_HOME_DIRECTORY} --rsynco=-aC  
			        \"sudo mkdir -p \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
			        sudo chown ${pipol_user} \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
			        cd \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
			        sh ${CMAKE_HOME_DIRECTORY}/buildtools/pipol/liste_install.sh \;
			        cmake -Denable_print_message=on -Denable_tracing=on -Denable_model-checking=on ${CMAKE_HOME_DIRECTORY} \;
			        make clean \;
			        make \;
			        make check \"
			        )
			      ADD_CUSTOM_TARGET(
			        ${SYSTEM_TARGET}_experimental
			        COMMENT "PIPOL Build : ${SYSTEM_PATTERN}_experimental"
			        COMMAND rsync ${pipol_user}@pipol.inria.fr:/usr/local/bin/pipol-sub .
			        COMMAND ./pipol-sub --pipol-user=${pipol_user} ${SYSTEM_PATTERN} 02:00 --verbose=1 --export=${CMAKE_HOME_DIRECTORY} --rsynco=-aC  
			        \"sudo mkdir -p \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
			        sudo chown ${pipol_user} \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
			        cd \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
			        sh ${CMAKE_HOME_DIRECTORY}/buildtools/pipol/liste_install.sh \;
			        cmake -Denable_tracing=on -Denable_model-checking=on ${CMAKE_HOME_DIRECTORY} \;
			        ctest -D Experimental \"
			        )
			        
			      STRING(REGEX MATCH "kvm" make_test "${SYSTEM_TARGET}")
			      if(make_test)
			      	STRING(REGEX MATCH "windows" make_test "${SYSTEM_TARGET}")
			      	if(NOT make_test)
	      			  ADD_CUSTOM_COMMAND(TARGET pipol_kvm_deploy
	      				COMMENT "PIPOL Build : ${SYSTEM_PATTERN}"
				        COMMAND rsync ${pipol_user}@pipol.inria.fr:/usr/local/bin/pipol-sub .
				        COMMAND ./pipol-sub --pipol-user=${pipol_user} ${SYSTEM_PATTERN} 02:00 --verbose=1 --export=${CMAKE_HOME_DIRECTORY} --rsynco=-aC  
				        \"sudo mkdir -p \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
				        sudo chown ${pipol_user} \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
				        cd \\\$$PIPOL_WDIR/${pipol_user}/${PROJECT_NAME} \;
				        sh ${CMAKE_HOME_DIRECTORY}/buildtools/pipol/liste_install.sh \;
				        cmake -Denable_tracing=on -Denable_model-checking=on ${CMAKE_HOME_DIRECTORY} \;
				        ctest -D Experimental \"
	        			 )
	        		endif(NOT make_test)
			      endif(make_test)
			      
			      ADD_CUSTOM_COMMAND(TARGET ${SYSTEM_TARGET}_experimental
			      					 POST_BUILD
			      					 COMMENT "See results on http://cdash.inria.fr/CDash/index.php?project=Simgrid"
			        				 )
			        				 
			      ADD_CUSTOM_COMMAND(TARGET pipol_test_list_images
					COMMAND echo ${SYSTEM_TARGET}
					)
				  ADD_CUSTOM_COMMAND(TARGET pipol_experimental_list_images
					COMMAND echo "${SYSTEM_TARGET}_experimental"
					)
      ENDMACRO(PIPOL_TARGET)
      
    ENDIF(HAVE_RSYNC)
    
# add a target for each pipol system
IF(PIPOL_SYSTEMS)
  MESSAGE(STATUS "Adding Pipol targets")
  FOREACH(SYSTEM ${PIPOL_SYSTEMS})
    PIPOL_TARGET(${SYSTEM})
  ENDFOREACH(SYSTEM ${PIPOL_SYSTEMS})
ENDIF(PIPOL_SYSTEMS)

ENDIF(pipol_user)