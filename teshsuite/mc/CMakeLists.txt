if (NOT SIMGRID_HAVE_MC)
  set(dwarf_disable 1)
  set(dwarf-expression_disable 1)
endif()


foreach(x dwarf dwarf-expression 
          random-bug mutex-handling)

  if(NOT DEFINED ${x}_sources)
    set(${x}_sources ${x}/${x}.cpp)
  endif()

  if(NOT DEFINED ${x}_disable)
    add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    set_property(TARGET ${x} APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
    add_dependencies(tests-mc ${x})
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)

  foreach(file ${${x}_sources})
    set(teshsuite_src  ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  endforeach()

  unset(${x}_sources)
  unset(${x}_disable)

endforeach()

add_executable       (without-mutex-handling EXCLUDE_FROM_ALL mutex-handling/mutex-handling.cpp)
target_link_libraries(without-mutex-handling simgrid)
set_target_properties(without-mutex-handling PROPERTIES COMPILE_FLAGS -DDISABLE_THE_MUTEX=1)
set_target_properties(without-mutex-handling PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mutex-handling)
add_dependencies(tests-mc without-mutex-handling)

set(teshsuite_src  ${teshsuite_src}                                                                        PARENT_SCOPE)
set(tesh_files     ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/random-bug/random-bug-nocrash.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/random-bug/random-bug-replay.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/mutex-handling/without-mutex-handling.tesh PARENT_SCOPE)

IF(SIMGRID_HAVE_MC)
  ADD_TESH(tesh-mc-dwarf                       --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/dwarf            --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/dwarf dwarf.tesh)
  ADD_TESH(tesh-mc-dwarf-expression            --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/dwarf-expression --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/dwarf-expression dwarf-expression.tesh)
  ADD_TESH(tesh-mc-mutex-handling              --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling mutex-handling.tesh --cfg=model-check/reduction:none)
# ADD_TESH(tesh-mc-mutex-handling-dpor         --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling mutex-handling.tesh --cfg=model-check/reduction:dpor)
  ADD_TESH(tesh-mc-without-mutex-handling      --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling without-mutex-handling.tesh --cfg=model-check/reduction:none)
  ADD_TESH(tesh-mc-without-mutex-handling-dpor --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling without-mutex-handling.tesh --cfg=model-check/reduction:dpor)
  IF("${CMAKE_SYSTEM}" MATCHES "Linux")
    ADD_TESH(mc-random-bug                       --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/random-bug --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/random-bug random-bug.tesh)
  ELSE()
    ADD_TESH(mc-random-bug-nocrash               --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/random-bug --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/random-bug random-bug-nocrash.tesh)
  ENDIF()
ENDIF()

ADD_TESH(mc-random-bug-replay                  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/random-bug --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/random-bug random-bug-replay.tesh)

if(enable_coverage)
  ADD_TEST(cover-mc-mutex-handling ${CMAKE_CURRENT_BINARY_DIR}/mutex-handling/mutex-handling ${CMAKE_HOME_DIRECTORY}/examples/platforms/small_platform.xml)
endif()
