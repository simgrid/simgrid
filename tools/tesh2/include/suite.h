#ifndef __SUITE_H	
#define __SUITE_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

suite_t
suite_new(unit_t owner, const char* description);

void
suite_include_unit(suite_t suite, unit_t unit);

void
suite_free(suite_t* suite);

#ifdef __cplusplus
extern }
#endif


#endif /* !__SUITE_H */

