/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning, this module is done to be efficient and performs tons of
   cast and dirty things. So avoid using it unless you really know
   what you are doing. */

/* This type should be added to a type that is to be used in such a swag */
/* Whenever a new object with this struct is created, all fields have to be swag to NULL */

typedef struct xbt_swag_hookup {
  void *next;
  void *prev;
} s_xbt_swag_hookup_t, *xbt_swag_hookup_t;

typedef struct xbt_swag {
  void *head;
  void *tail;
  size_t offset;
  int count;
} s_xbt_swag_t, *xbt_swag_t;

xbt_swag_t xbt_swag_new(size_t offset);
void xbt_swag_init(xbt_swag_t swag, size_t offset);
void  xbt_swag_insert(void *obj,xbt_swag_t swag);
void *xbt_swag_extract(void *obj, xbt_swag_t swag);
int  xbt_swag_size(xbt_swag_t swag);
int  xbt_swag_belongs(void *obj,xbt_swag_t swag);

static __inline__ void *xbt_swag_getFirst(xbt_swag_t swag)
{
  return(swag->head);
}

#define xbt_swag_getNext(obj,offset) (((xbt_swag_hookup_t)(((char *) (obj)) + (offset)))->prev)
#define xbt_swag_getPrev(obj,offset) (((xbt_swag_hookup_t)(((char *) (obj)) + (offset)))->next)

#define xbt_swag_offset(var,field) ((char *)&( (var).field ) - (char *)&(var)) 

#define xbt_swag_foreach(obj,swag)                            \
   for((obj)=xbt_swag_getFirst((swag));                           \
       (obj)!=NULL;                                           \
       (obj)=xbt_swag_getNext((obj),(swag)->offset))
