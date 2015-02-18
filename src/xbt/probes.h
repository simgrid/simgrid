/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef USE_UST
#  include <lttng/tracepoint.h>
#  include "simgrid_ust.h"
#  define XBT_TRACE0 tracepoint
#  define XBT_TRACE1 tracepoint
#  define XBT_TRACE2 tracepoint
#  define XBT_TRACE3 tracepoint
#  define XBT_TRACE4 tracepoint
#  define XBT_TRACE5 tracepoint
#  define XBT_TRACE6 tracepoint
#  define XBT_TRACE7 tracepoint
#  define XBT_TRACE8 tracepoint
#  define XBT_TRACE9 tracepoint
#  define XBT_TRACE10 tracepoint
#  define XBT_TRACE11 tracepoint
#  define XBT_TRACE12 tracepoint
#elif USE_SDT
#  include <sys/sdt.h>
#  define XBT_TRACE0  DTRACE_PROBE0
#  define XBT_TRACE1  DTRACE_PROBE1
#  define XBT_TRACE2  DTRACE_PROBE2
#  define XBT_TRACE3  DTRACE_PROBE3
#  define XBT_TRACE4  DTRACE_PROBE4
#  define XBT_TRACE5  DTRACE_PROBE5
#  define XBT_TRACE6  DTRACE_PROBE6
#  define XBT_TRACE7  DTRACE_PROBE7
#  define XBT_TRACE8  DTRACE_PROBE8
#  define XBT_TRACE9  DTRACE_PROBE9
#  define XBT_TRACE10  DTRACE_PROBE10
#  define XBT_TRACE11  DTRACE_PROBE11
#  define XBT_TRACE12  DTRACE_PROBE12
#else
#  define XBT_TRACE0(...)
#  define XBT_TRACE1(...)
#  define XBT_TRACE2(...)
#  define XBT_TRACE3(...)
#  define XBT_TRACE4(...)
#  define XBT_TRACE5(...)
#  define XBT_TRACE6(...)
#  define XBT_TRACE7(...)
#  define XBT_TRACE8(...)
#  define XBT_TRACE9(...)
#  define XBT_TRACE10(...)
#  define XBT_TRACE11(...)
#  define XBT_TRACE12(...)
#endif
