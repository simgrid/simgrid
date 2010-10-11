#ifndef __VARIABLE_H
#define __VARIABLE_H

#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   variable_t  variable_new(const char *name, const char *val);
  int  variable_free(variable_t * variableptr);
    int  variable_is_used(variable_t variable);
    int  variable_set_used(variable_t variable);
    
#ifdef __cplusplus
} 
#endif  /*  */
 
#endif  /*!__VARIABLE_H */

