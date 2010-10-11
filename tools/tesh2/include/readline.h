#ifndef __READLINE_H	
#define __READLINE_H

#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   long  readline(FILE * stream, char **buf, size_t * n);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__READLINE_H */

