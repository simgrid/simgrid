
#ifndef GRAS_MODULE_H
#define GRAS_MODULE_H

#include <xbt.h>

/* Function users of module should use */
XBT_PUBLIC(void) gras_module_join(const char *name);
XBT_PUBLIC(void) gras_module_leave(const char *name);


/* Functions module implementor should use */
XBT_PUBLIC(void) gras_module_add(const char *name, unsigned int data_size,
                                 int *ID, void_f_void_t init_f,
                                 void_f_void_t exit_f, void_f_pvoid_t join_f,
                                 void_f_pvoid_t leave_f);


XBT_PUBLIC(void *) gras_moddata_by_id(unsigned int ID);


#endif /* GRAS_MODULE_H */
