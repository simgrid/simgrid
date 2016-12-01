find_path(LIBEVENT_INCLUDE_DIR event2/event.h)
find_library(LIBEVENT_LIBRARY NAMES event)
find_library(LIBEVENT_THREADS_LIBRARY NAMES event_pthreads)
set(LIBEVENT_LIBRARIES "${LIBEVENT_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Libevent
    DEFAULT_MSG
    LIBEVENT_LIBRARIES
    LIBEVENT_INCLUDE_DIR)
mark_as_advanced(LIBEVENT_INCLUDE_DIR LIBEVENT_LIBRARIES)
