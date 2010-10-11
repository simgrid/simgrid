#ifndef __UNITS_H	
#define __UNITS_H

#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   units_t  units_new(runner_t runner, fstreams_t fstreams);
    int  units_is_empty(units_t unit);
    int  units_get_size(units_t unit);
    int  units_run_all(units_t units, xbt_os_mutex_t mutex);
    int  units_join_all(units_t units);
    int  units_interrupt_all(units_t units);
    int  units_reset_all(units_t units);
     int  units_summuarize(units_t units);
    int  units_free(void **unitsptr);
  
#ifdef __cplusplus
extern} 
#endif  /*  */
 
#endif  /* !__UNITS_H */
 
