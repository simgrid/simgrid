#ifndef __VARIABLES_H
#define __VARIABLES_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

variables_t
variables_new(void);

int
variables_free(variables_t* variablesptr);


#ifdef __cplusplus
}
#endif


#endif /*!__VARIABLES_H */