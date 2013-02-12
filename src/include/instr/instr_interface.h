#include "xbt.h"

XBT_PUBLIC(int) TRACE_start (void);
XBT_PUBLIC(int) TRACE_end (void);
XBT_PUBLIC(void) TRACE_global_init(int *argc, char **argv);
XBT_PUBLIC(void) TRACE_help(int detailed);
XBT_PUBLIC(void) TRACE_surf_resource_utilization_alloc(void);
XBT_PUBLIC(void) TRACE_surf_resource_utilization_release(void);
XBT_PUBLIC(void) TRACE_add_start_function(void (*func)(void));
XBT_PUBLIC(void) TRACE_add_end_function(void (*func)(void));
