#ifndef __DIRECTORY_H
#define __DIRECTORY_H

#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   directory_t  directory_new(const char *name);
  int  directory_open(directory_t directory);
    int  directory_close(directory_t directory);
    int 
      directory_load(directory_t directory, fstreams_t fstreams,
                     xbt_dynar_t suffixes);
    int  directory_free(void **directoryptr);
  const char * directory_get_name(directory_t directory);
    
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /*!__DIRECTORY_H */
