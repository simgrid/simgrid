find_path(HAVE_CGRAPH_H cgraph.h
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES include/graphviz include
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_path(HAVE_AGRAPH_H agraph.h
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES include/graphviz include
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_path(HAVE_GRAPH_H graph.h
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES include/graphviz include
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_library(HAVE_CGRAPH_LIB
    NAME cgraph
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lib/graphviz lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)  

find_library(HAVE_AGRAPH_LIB
    NAME agraph
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lib/graphviz lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
) 

find_library(HAVE_GRAPH_LIB
    NAME graph
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lib/graphviz lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_library(HAVE_CDT_LIB
    NAME cdt
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lib/graphviz lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)