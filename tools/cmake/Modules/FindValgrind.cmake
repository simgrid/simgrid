find_program(VALGRIND_EXE
  NAME valgrind
  PATH_SUFFIXES bin/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )
mark_as_advanced(VALGRIND_EXE)
