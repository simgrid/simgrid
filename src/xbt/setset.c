#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "setset_private.h"
#include "xbt/sysdep.h"
#include "simgrid_config.h" /*_XBT_WIN32*/

/*The function ffs doesn't exist for windows*/
#ifdef _XBT_WIN32
int ffs(int bits)
{
  int i;
  if (bits == 0)
    return (0);
  for (i = 1;; i++, bits >>= 1) {
    if (bits & 1)
      break;
  }
  return (i);
}
#endif

/**
 *  \brief Create a new setset data structure
 *  \param size The initial size of the setset (in number of elements)
 *  \return The created setset
 */
xbt_setset_t xbt_setset_new(unsigned int size)
{
  xbt_setset_elm_entry_t first_elm = NULL;
  xbt_setset_t setset = xbt_new0(s_xbt_setset_t, 1);
  setset->elm_array =
      xbt_dynar_new(sizeof(u_xbt_setset_elm_entry_t), NULL);
  setset->sets = xbt_fifo_new();
  /* Expand the elements dynar to the size indicated by the user, */
  /* then create the first element, get a pointer to it and add it to the */
  /* free elements list */
  xbt_dynar_shrink(setset->elm_array, size);
  first_elm =
      (xbt_setset_elm_entry_t) xbt_dynar_push_ptr(setset->elm_array);
  first_elm->free.next = 0;
  return setset;
}

/**
 *  \brief Destroy a setset and free all it's resources
 *  \param The setset to destroy
 */
void xbt_setset_destroy(xbt_setset_t setset)
{
  xbt_fifo_item_t item;
  xbt_setset_set_t set;
  xbt_dynar_free(&setset->elm_array);
  xbt_fifo_foreach(setset->sets, item, set, xbt_setset_set_t) {
    xbt_setset_destroy_set(set);
  }
  xbt_fifo_free(setset->sets);
  xbt_free(setset);
}

/* Add an object to the setset, this will calculate its ID */
void xbt_setset_elm_add(xbt_setset_t setset, void *obj)
{
  xbt_setset_elm_entry_t new_entry = NULL;
  xbt_setset_elm_entry_t first_elm = NULL;
  xbt_setset_elm_t e = (xbt_setset_elm_t) obj;
  xbt_assert0(e->ID == 0, "Adding element with non NULL ID");
  first_elm =
      (xbt_setset_elm_entry_t) xbt_dynar_get_ptr(setset->elm_array, 0);

  /* Before create a new elm entry check if there is one in the free elm list. */
  /* If there is not free elm entries, then create a new one  */
  if (first_elm->free.next != 0) {
    e->ID = first_elm->free.next;
    new_entry =
        (xbt_setset_elm_entry_t) xbt_dynar_get_ptr(setset->elm_array,
                                                   first_elm->free.next);
    first_elm->free.next = new_entry->free.next;
  } else {
    new_entry =
        (xbt_setset_elm_entry_t) xbt_dynar_push_ptr(setset->elm_array);
    e->ID = xbt_dynar_length(setset->elm_array) - 1;
  }

  new_entry->info.obj = e;
  return;
}

/* Remove an object from the setset */
void xbt_setset_elm_remove(xbt_setset_t setset, void *obj)
{
  xbt_setset_elm_t e = (xbt_setset_elm_t) obj;
  xbt_setset_elm_entry_t e_entry =
      xbt_dynar_get_ptr(setset->elm_array, e->ID);
  xbt_setset_elm_entry_t first_free = NULL;

  /* Link the elm entry to the list of free ones */
  first_free = xbt_dynar_get_ptr(setset->elm_array, 0);
  e_entry->free.next = first_free->free.next;
  first_free->free.next = e->ID;
}

/* Get the object associated to a given index */
/* WARNING: it must be a valid index! */
void *_xbt_setset_idx_to_obj(xbt_setset_t setset, unsigned long idx)
{
  xbt_setset_elm_entry_t e_entry =
      xbt_dynar_get_ptr(setset->elm_array, idx);
  return e_entry->info.obj;
}

/**
 *  \brief Add a new set to the setset
 *  \param setset The setset that will contain the created set
 *  \returns The created set
 */
xbt_setset_set_t xbt_setset_new_set(xbt_setset_t setset)
{
  xbt_setset_set_t newset = xbt_new0(s_xbt_setset_set_t, 1);
  newset->setset = setset;
  newset->size = xbt_dynar_length(setset->elm_array) / BITS_INT + 1;
  newset->bitmap = xbt_new0(unsigned int, newset->size);
  xbt_fifo_unshift(setset->sets, newset);
  return newset;
}

/**
 *  \brief Destroy a set in the setset
 *  \param set The set to destroy
 */
void xbt_setset_destroy_set(xbt_setset_set_t set)
{
  xbt_free(set->bitmap);
  xbt_fifo_remove(set->setset->sets, set);
  xbt_free(set);

  return;
}

/**
 *  \brief Insert an element into a set
 *  \param set The set where the element is going to be added
 *  \param obj The element to add
 */
void xbt_setset_set_insert(xbt_setset_set_t set, void *obj)
{
  xbt_setset_elm_t e = (xbt_setset_elm_t) obj;

  if (e->ID == 0)
    xbt_setset_elm_add(set->setset, e);

  /* Check if we need to expand the bitmap */
  if (set->size * BITS_INT - 1 < e->ID) {
    set->bitmap =
        xbt_realloc(set->bitmap, (e->ID / BITS_INT + 1) * sizeof(int));
    memset(&set->bitmap[set->size], 0,
           ((e->ID / BITS_INT + 1) - set->size) * sizeof(int));
    set->size = (e->ID / BITS_INT + 1);
  }

  _set_bit(e->ID, set->bitmap);

  return;
}

/**
 *  \brief Remove an element from a set
 *  \param set The set from which the element is going to be removed
 *  \param obj The element to remove
 */
void xbt_setset_set_remove(xbt_setset_set_t set, void *obj)
{
  xbt_setset_elm_t e = (xbt_setset_elm_t) obj;
  /* If the index of the object is between the bitmap then unset it, otherwise
     do not do anything, because we already know it is not in the set */
  if (e->ID != 0 && e->ID <= set->size * BITS_INT)
    _unset_bit(e->ID, set->bitmap);

  return;
}

/**
 *  \brief Remove all the elements of a set
 *  \param set The set to empty
 */
void xbt_setset_set_reset(xbt_setset_set_t set)
{
  memset(set->bitmap, 0, set->size * sizeof(int));
}

/**
 *  \brief Choose one element of a set (but do not remove it)
 *  \param set The set
 *  \return An element that belongs to set \a set
 */
void *xbt_setset_set_choose(xbt_setset_set_t set)
{
  int i;
  /* Traverse the set and return the first element */
  for (i = 0; i < set->size; i++)
    if (set->bitmap[i] != 0)
      return _xbt_setset_idx_to_obj(set->setset,
                                    i * BITS_INT + ffs(set->bitmap[i]) -
                                    1);
  return NULL;
}

/**
 *  \brief Extract one element of a set (it removes it)
 *  \param set The set
 *  \return An element that belonged to set \a set
 */
void *xbt_setset_set_extract(xbt_setset_set_t set)
{
  void *obj = xbt_setset_set_choose(set);
  if (obj) {
    xbt_setset_set_remove(set, obj);
  }
  return obj;
}


/**
 *  \brief Test if an element belongs to a set
 *  \param set The set
 *  \param obj The element
 *  \return TRUE if the element \a obj belongs to set \a set
 */
int xbt_setset_set_belongs(xbt_setset_set_t set, void *obj)
{
  xbt_setset_elm_t e = (xbt_setset_elm_t) obj;
  if (e->ID != 0 && e->ID <= set->size * BITS_INT) {
    return _is_bit_set(e->ID % BITS_INT, set->bitmap[e->ID / BITS_INT]);
  }
  return FALSE;
}

int xbt_setset_set_size(xbt_setset_set_t set)
{
  int i, count = 0;

  for (i = 0; i < set->size; i++)
    count += bitcount(set->bitmap[i]);

  return count;
}


/**
 *  \brief Add two sets
 *         Add two sets storing the result in the first one
 *  \param set1 The first set
 *  \param set2 The second set
 */
void xbt_setset_add(xbt_setset_set_t set1, xbt_setset_set_t set2)
{
  int i;

  /* Increase the size of set1 if necessary */
  if (set1->size < set2->size) {
    xbt_realloc(set1->bitmap, set2->size * sizeof(unsigned int));
    set1->size = set2->size;
  }

  for (i = 0; i < set1->size; i++)
    if (set2->bitmap[i] != 0)
      set1->bitmap[i] |= set2->bitmap[i];

  return;
}

/**
 *  \brief Substract two sets
 *         Substract two sets storing the result in the first one
 *  \param set1 The first set
 *  \param set2 The second set
 */
void xbt_setset_substract(xbt_setset_set_t set1, xbt_setset_set_t set2)
{
  int i;

  for (i = 0; i < MIN(set1->size, set2->size); i++)
    if (set2->bitmap[i] != 0)
      set1->bitmap[i] ^= set2->bitmap[i];

  return;
}

/**
 *  \brief Intersect two sets
 *         Intersect two sets storing the result in the first one
 *  \param set1 The first set
 *  \param set2 The second set
 */
void xbt_setset_intersect(xbt_setset_set_t set1, xbt_setset_set_t set2)
{
  int i;

  for (i = 0; i < MIN(set1->size, set2->size); i++)
    if (set1->bitmap[i] && set2->bitmap[i])
      set1->bitmap[i] &= set2->bitmap[i];

  return;
}

/* Get a cursor pointing to the first element of the set */
void xbt_setset_cursor_first(xbt_setset_set_t set,
                             xbt_setset_cursor_t * cursor)
{
  int i;
  (*cursor) = xbt_new0(s_xbt_setset_cursor_t, 1);
  (*cursor)->set = set;

  for (i = 0; i < set->size; i++) {
    if (set->bitmap[i] != 0) {
      (*cursor)->idx = i * BITS_INT + ffs(set->bitmap[i]) - 1;
      break;
    }
  }
}

/* Get the data pointed by a cursor */
int xbt_setset_cursor_get_data(xbt_setset_cursor_t cursor, void **data)
{
  if (cursor->idx == 0) {
    xbt_free(cursor);
    *data = NULL;
    return FALSE;
  } else {
    *data = _xbt_setset_idx_to_obj(cursor->set->setset, cursor->idx);
    return TRUE;
  }
}

/* Advance a cursor to the next element */
void xbt_setset_cursor_next(xbt_setset_cursor_t cursor)
{
  unsigned int mask;
  unsigned int data;
  cursor->idx++;
  while (cursor->idx < cursor->set->size * BITS_INT) {
    if ((data = cursor->set->bitmap[cursor->idx / BITS_INT])) {
      mask = 1 << cursor->idx % BITS_INT;
      while (mask) {            /* FIXME: mask will never be 0! */
        if (data & mask) {
          return;
        } else {
          cursor->idx++;
          mask <<= 1;
        }
      }
    } else {
      cursor->idx += BITS_INT;
    }
  }
  cursor->idx = 0;
}

/* Check if the nth bit of an integer is set or not*/
unsigned int _is_bit_set(unsigned int bit, unsigned int integer)
{
  return (0x1 << bit) & integer ? TRUE : FALSE;
}

/* Set the nth bit of an array of integers */
void _set_bit(unsigned int bit, unsigned int *bitmap)
{
  bitmap[bit / BITS_INT] |= 0x1 << (bit % BITS_INT);
}

/* Unset the nth bit of an array of integers */
void _unset_bit(unsigned int bit, unsigned int *bitmap)
{
  bitmap[bit / BITS_INT] &= ~(0x1 << (bit % BITS_INT));
}

/**
 * Bitcount function 
 * taken from http://graphics.stanford.edu/~seander/bithacks.html 
 * Note: it assumes 4 byte integers
 */
int bitcount(int v)
{
  v = v - ((v >> 1) & 0x55555555);      // reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);       // temp
  return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;      // count
}
