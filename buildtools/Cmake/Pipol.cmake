# On a Pipol system, set:
# PIPOL_USER

#SET(PIPOL_USER $ENV{PIPOL_USER})

# ssh/rsync mandatory 
IF(pipol_user)

	set(CMAKE_OPTIONS "	-Wno-dev")
	
	if(custom_flags)
		set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Dcustom_flags=${custom_flags}")
	endif(custom_flags)
	
	if(with_context)
		set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Dwith_context=${with_context}")
	endif(with_context)
	
	if(enable_smpi)
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_smpi=on")
	endif(enable_smpi)
	
	if(enable_java)
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_java=on")
	endif(enable_java)
	
	if(enable_lua)
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_lua=on")
	endif(enable_lua)	
	
	if(enable_ruby)
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_ruby=on")
	endif(enable_ruby)	
	
	if(enable_compile_optimizations)
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_compile_optimizations=on")
	endif(enable_compile_optimizations)
	
	if(enable_compile_warnings)
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_compile_warnings=on")
	endif(enable_compile_warnings)	
	
	if(enable_tracing)
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_tracing=on")
	endif(enable_tracing)
	
	if(enable_coverage)
	set(CMAKE_OPTIONS "${CMAKE_OPTION}	-Denable_coverage=on")
	endif(enable_coverage)	
	
	if(enable_print_message)	
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_print_message=on")
	endif(enable_print_message)	
	
	if(enable_model-checking)
	set(CMAKE_OPTIONS "${CMAKE_OPTIONS}	-Denable_model-checking=on")
	endif(enable_model-checking)

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
			        \"sudo chown ${pipol_user} ${CMAKE_HOME_DIRECTORY} \;
			        cd ${CMAKE_HOME_DIRECTORY} \;
			        sh ${CMAKE_HOME_DIRECTORY}/buildtools/pipol/liste_install.sh \;
			        cmake -E remove CMakeCache.txt \;
			        cmake ${CMAKE_HOME_DIRECTORY}${CMAKE_OPTIONS} \;
			        make clean \;
			        make \;
			        make check \"
			        )
			      ADD_CUSTOM_TARGET(
			        ${SYSTEM_TARGET}_experimental
			        COMMENT "PIPOL Build : ${SYSTEM_PATTERN}_experimental"
			        COMMAND rsync ${pipol_user}@pipol.inria.fr:/usr/local/bin/pipol-sub .
			        COMMAND ./pipol-sub --pipol-user=${pipol_user} ${SYSTEM_PATTERN} 02:00 --reconnect --group --keep --verbose=1 --export=${CMAKE_HOME_DIRECTORY} --rsynco=-aC  
			        \"sudo chown ${pipol_user} ${CMAKE_HOME_DIRECTORY} \;
			        cd ${CMAKE_HOME_DIRECTORY} \;
			        sh ${CMAKE_HOME_DIRECTORY}/buildtools/pipol/liste_install.sh \;
			        cmake -E remove CMakeCache.txt \;
			        cmake ${CMAKE_HOME_DIRECTORY}${CMAKE_OPTIONS} \;
			        ctest -D Experimental \"
			        )
			        
			      STRING(REGEX MATCH "kvm" make_test "${SYSTEM_TARGET}")
			      if(make_test)
			      	STRING(REGEX MATCH "windows" make_test "${SYSTEM_TARGET}")
			      	if(NOT make_test)
	      			  ADD_CUSTOM_COMMAND(TARGET pipol_kvm_deploy
	      				COMMENT "PIPOL Build : ${SYSTEM_PATTERN}"
				        COMMAND rsync ${pipol_user}@pipol.inria.fr:/usr/local/bin/pipol-sub .
				        COMMAND ./pipol-sub --pipol-user=${pipol_user} ${SYSTEM_PATTERN} 02:00 --reconnect --group --keep --verbose=1 --export=${CMAKE_HOME_DIRECTORY} --rsynco=-aC  
				        \"sudo chown ${pipol_user} ${CMAKE_HOME_DIRECTORY} \;
				        cd ${CMAKE_HOME_DIRECTORY} \;
				        sh ${CMAKE_HOME_DIRECTORY}/buildtools/pipol/liste_install.sh \;
				        cmake -E remove CMakeCache.txt \;
				        cmake ${CMAKE_HOME_DIRECTORY}${CMAKE_OPTIONS} \;
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

ADD_CUSTOM_TARGET(pipol_kill_all_jobs
		COMMENT "PIPOL delete all jobs"
		COMMAND ./pipol-sub --pipol-user=${pipol_user} deleteallmyjobs
		)


ENDIF(pipol_user)