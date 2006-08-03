#ifndef _XBT_MALLOCATOR_H
#define _XBT_MALLOCATOR_H

#include "xbt/function_types.h"
#include "xbt/misc.h" /* SG_BEGIN_DECL */

SG_BEGIN_DECL()

/* mallocator data type (opaque structure) */
typedef struct s_xbt_mallocator *xbt_mallocator_t;

/* creation and destruction */
xbt_mallocator_t xbt_mallocator_new(int size, pvoid_f_void_t new_f, void_f_pvoid_t free_f, void_f_pvoid_t reset_f);
void xbt_mallocator_free(xbt_mallocator_t mallocator);

/* object handling */
void *xbt_mallocator_get(xbt_mallocator_t mallocator);
void xbt_mallocator_release(xbt_mallocator_t mallocator, void *object);

SG_END_DECL()

#endif /* _XBT_MALLOCATOR_H */
