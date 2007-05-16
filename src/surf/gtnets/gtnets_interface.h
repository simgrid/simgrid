// Interface for GTNetS.
// Kayo Fujiwara 1/8/2007

#ifndef _GTNETS_INTERFACE_H
#define _GTNETS_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif
  
  int gtnets_initialize();
  int gtnets_add_link(int id, double bandwidth, double latency);
  int gtnets_add_route(int src, int dst, int* links, int nlink);
  int gtnets_create_flow(int src, int dst, long datasize, void* metadata);
  double gtnets_get_time_to_next_flow_completion();
  int gtnets_run_until_next_flow_completion(void*** metadata, int* number_of_flows);
  int gtnets_run(double delta);
  int gtnets_finalize();

#ifdef __cplusplus
}
#endif

#endif


