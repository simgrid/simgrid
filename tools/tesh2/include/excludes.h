#ifndef __EXCLUDES_H
#define __EXCLUDES_H

#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   excludes_t  excludes_new(void);
  int  excludes_add(excludes_t excludes, fstream_t fstream);
    int  excludes_contains(excludes_t excludes, fstream_t fstream);
    int  excludes_is_empty(excludes_t excludes);
    int  excludes_check(excludes_t excludes, fstreams_t fstreams);
    int  excludes_free(void **excludesptr);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /*!__EXCLUDES_H */
