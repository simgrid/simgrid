#ifndef _XBT_SETSET_H
#define _XBT_SETSET_H
#include "xbt/misc.h"

typedef struct s_xbt_setset *xbt_setset_t;

typedef struct s_xbt_setset_set *xbt_setset_set_t;

typedef struct s_xbt_setset_cursor *xbt_setset_cursor_t;

#define XBT_SETSET_HEADERS \
  unsigned long ID

/* Constructor */
xbt_setset_t xbt_setset_new(unsigned int size);

/* Destructor */
void xbt_setset_destroy(xbt_setset_t setset);

/* Add an object to the setset, this will calculate its ID */
void xbt_setset_elm_add(xbt_setset_t setset, void *obj);

/* Remove an object from the setset */
void xbt_setset_elm_remove(xbt_setset_t setset, void * obj);

/* Create a new set in the setset */
xbt_setset_set_t xbt_setset_new_set(xbt_setset_t setset);

/* Destroy a set in the setset */
void xbt_setset_destroy_set(xbt_setset_set_t);

/* Insert an element into a set */
void xbt_setset_set_insert(xbt_setset_set_t set, void* obj);

/* Remove an element from a set */
void xbt_setset_set_remove(xbt_setset_set_t set, void* obj);

/* Remove all the elements of a set */
void xbt_setset_set_reset(xbt_setset_set_t set);

/* Select one element of a set */
void *xbt_setset_set_choose(xbt_setset_set_t set);

/* Extract one element of a set */
void *xbt_setset_set_extract(xbt_setset_set_t set);

/* Test if an element belongs to a set */
int xbt_setset_set_belongs(xbt_setset_set_t set, void* obj);

/* Get the number of elements in a set */
int xbt_setset_set_size(xbt_setset_set_t set);

/* Add all elements of set2 to set1 */
void xbt_setset_add(xbt_setset_set_t set1, xbt_setset_set_t set2);

/* Substract all elements of set2 from set1 */
void xbt_setset_substract(xbt_setset_set_t set1, xbt_setset_set_t set2);

/* Intersect set1 and set2 storing the result in set1 */
void xbt_setset_intersect(xbt_setset_set_t set1, xbt_setset_set_t set2);

/* Get the cursor to point to the first element of a set */
void xbt_setset_cursor_first(xbt_setset_set_t set, xbt_setset_cursor_t *cursor);

/* Get the data pointed by a cursor */
int xbt_setset_cursor_get_data(xbt_setset_cursor_t cursor, void **data);

/* Advance a cursor to the next element */
void xbt_setset_cursor_next(xbt_setset_cursor_t cursor);


#define xbt_setset_foreach(set, cursor, data) \
          for(xbt_setset_cursor_first(set, &cursor);  \
              xbt_setset_cursor_get_data(cursor, (void **)&data); \
              xbt_setset_cursor_next(cursor))  

#endif