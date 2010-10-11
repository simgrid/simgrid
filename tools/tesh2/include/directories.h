#ifndef __DIRECTORIES_H
#define __DIRECTORIES_H

#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   directories_t  directories_new(void);
  int 
      directories_add(directories_t directories, directory_t directory);
    int 
      directories_contains(directories_t directories,
                           directory_t directory);
    int  directories_load(directories_t directories,
                             fstreams_t fstreams, xbt_dynar_t suffixes);
    int  directories_free(void **directoriesptr);
  int  directories_get_size(directories_t directories);
    int  directories_is_empty(directories_t directories);
   
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /*!__DIRECTORIES_H */
