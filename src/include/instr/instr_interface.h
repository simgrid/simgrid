int TRACE_start (void);
int TRACE_end (void);
void TRACE_global_init(int *argc, char **argv);
void TRACE_help(int detailed);
void TRACE_surf_resource_utilization_alloc(void);
void TRACE_surf_resource_utilization_release(void);
void TRACE_add_start_function(void (*func)(void));
void TRACE_add_end_function(void (*func)(void));
