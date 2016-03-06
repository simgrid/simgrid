find_path(HAVE_CGRAPH_H cgraph.h
  HINTS  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES include/graphviz include
  PATHS         /opt;/opt/local;/opt/csw;/sw;/usr
)

find_library(HAVE_CGRAPH_LIB
  NAME          cgraph
  HINTS         $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/graphviz lib
  PATHS         /opt;/opt/local;/opt/csw;/sw;/usr)

find_library(HAVE_CDT_LIB
  NAME          cdt
  HINTS         $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/graphviz lib
  PATHS         /opt;/opt/local;/opt/csw;/sw;/usr)


if(HAVE_CDT_LIB AND HAVE_CGRAPH_LIB AND HAVE_CGRAPH_H)
  string(REGEX REPLACE "/libcgraph.*" "" lib_graphviz ${HAVE_CGRAPH_LIB})

  string(REPLACE "/graphviz/cgraph.h" "" file_graphviz_h ${HAVE_CGRAPH_H})
  string(REPLACE "/graphviz" "" file_graphviz_h ${file_graphviz_h})

  include_directories(${file_graphviz_h} ${file_graphviz_h}/graphviz)
  link_directories(${lib_graphviz})

  set(HAVE_GRAPHVIZ "1")
else()
  set(HAVE_GRAPHVIZ "0")
endif()

mark_as_advanced(HAVE_GRAPHVIZ)
unset(HAVE_CGRAPH_H)
unset(HAVE_CGRAPH_LIB)
unset(HAVE_CDT_LIB)

message(STATUS "Looking for graphviz")
if(HAVE_GRAPHVIZ)
  message(STATUS "Looking for graphviz - found")
else()
  message(STATUS "Looking for graphviz - not found")
endif()
