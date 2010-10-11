#ifndef __STR_REPLACE_H
#define __STR_REPLACE_H

#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  int 
      str_replace(char **str, const char *what, const char *with,
                  const char *delimiters);
  int  str_replace_all(char **str, const char *what, const char *with,
                          const char *delimiters);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__STR_REPLACE_H */
