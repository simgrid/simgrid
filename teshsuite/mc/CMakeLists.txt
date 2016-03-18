foreach(x dwarf dwarf-expression)
  if (HAVE_MC)
    add_executable       (${x}  ${x}/${x}.cpp)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

    ADD_TESH(tesh-mc-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/${x} ${x}.tesh)
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

add_executable       (without-mutex-handling mutex-handling/mutex-handling.c)
target_link_libraries(without-mutex-handling simgrid)
set_target_properties(without-mutex-handling PROPERTIES COMPILE_FLAGS -DDISABLE_THE_MUTEX=1)
set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mutex-handling)

foreach(x mutex-handling random-bug)
  add_executable       (${x} ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

set(teshsuite_src  ${teshsuite_src}                                                                        PARENT_SCOPE)
set(tesh_files     ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/random-bug/random-bug-report.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/mutex-handling/without-mutex-handling.tesh PARENT_SCOPE)
set(xml_files      ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/mutex-handling/mutex-handling_d.xml        PARENT_SCOPE)

IF(HAVE_MC)
  ADD_TESH(tesh-mc-mutex-handling              --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling mutex-handling.tesh --cfg=model-check/reduction:none)
# ADD_TESH(tesh-mc-mutex-handling-dpor         --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling mutex-handling.tesh --cfg=model-check/reduction:dpor)
  ADD_TESH(tesh-mc-without-mutex-handling      --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling without-mutex-handling.tesh --cfg=model-check/reduction:none)
  ADD_TESH(tesh-mc-without-mutex-handling-dpor --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling without-mutex-handling.tesh --cfg=model-check/reduction:dpor)

  ADD_TESH(mc-random-bug-record                --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/random-bug --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/random-bug random-bug-report.tesh)
ENDIF()

ADD_TESH(mc-random-bug                         --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/random-bug --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/random-bug random-bug.tesh)
