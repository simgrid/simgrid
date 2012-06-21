cmake_minimum_required(VERSION 2.6)

set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/compile-stats
  ${CMAKE_CURRENT_SOURCE_DIR}/fix-paje-trace.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/indent
  ${CMAKE_CURRENT_SOURCE_DIR}/sg_unit_extractor.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/simgrid_update_xml.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/xbt_exception_checker
  ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/index_create.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/fig2dev_postprocessor.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/xbt_log_extract_hierarchy.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/MSG_visualization/colorize.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/MSG_visualization/trace2fig.pl
  PARENT_SCOPE
)