#ifndef MMALLOC_H
#define MMALLOC_H 1

#ifdef HAVE_STDDEF_H
#  include <stddef.h>
#else
#  include <sys/types.h>        /* for size_t */
#  include <stdio.h>            /* for NULL */
#endif
//#include "./include/ansidecl.h"

/* Allocate SIZE bytes of memory.  */

extern void *mmalloc(void *md, size_t size);

/* Re-allocate the previously allocated block in void*, making the new block
   SIZE bytes long.  */

extern void *mrealloc(void *md, void *ptr, size_t size);

/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0.  */

extern void *mcalloc(void *md, size_t nmemb, size_t size);

/* Free a block allocated by `mmalloc', `mrealloc' or `mcalloc'.  */

extern void mfree(void *md, void *ptr);

/* Allocate SIZE bytes allocated to ALIGNMENT bytes.  */

extern void *mmemalign(void *md, size_t alignment, size_t size);

/* Allocate SIZE bytes on a page boundary.  */

extern void *mvalloc(void *md, size_t size);

/* Activate a standard collection of debugging hooks.  */

extern int mmcheck(void *md, void (*func) (void));

extern int mmcheckf(void *md, void (*func) (void), int force);

/* Pick up the current statistics. (see FIXME elsewhere) */

extern struct mstats mmstats(void *md);

extern void *mmalloc_attach(int fd, void *baseaddr);

extern void mmalloc_pre_detach(void *md);

extern void *mmalloc_detach(void *md);

extern int mmalloc_setkey(void *md, int keynum, void *key);

extern void *mmalloc_getkey(void *md, int keynum);

// FIXME: this function is not implemented anymore?
//extern int mmalloc_errno (void* md);

/* return the heap used when NULL is passed as first argument to any mm* function */
extern void *mmalloc_get_default_md(void);

extern int mmtrace(void);

extern void *mmalloc_findbase(int size);

/* To change the heap used when using the legacy version malloc/free/realloc and such */
void mmalloc_set_current_heap(void *new_heap);
void *mmalloc_get_current_heap(void);



#endif                          /* MMALLOC_H */
