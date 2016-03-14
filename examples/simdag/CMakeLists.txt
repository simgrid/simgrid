foreach(x availability fail typed_tasks io properties comm_throttling scheduling test)
  add_executable       (sd_${x}     ${x}/sd_${x}.c)
  target_link_libraries(sd_${x}     simgrid)
  set_target_properties(sd_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.tesh)

  ADD_TESH(simdag-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/simdag --cd ${CMAKE_BINARY_DIR}/examples/simdag ${CMAKE_HOME_DIRECTORY}/examples/simdag/${x}/sd_${x}.tesh)
endforeach()

set(examples_src ${examples_src}                                                           PARENT_SCOPE)
set(tesh_files   ${tesh_files}                                                             PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/scheduling/Montage_25.xml    PARENT_SCOPE)
set(bin_files    ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/platform_script.lua          PARENT_SCOPE)
