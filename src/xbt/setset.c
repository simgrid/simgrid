#include <stddef.h>
#include <stdio.h>
#include "setset_private.h"
#include "xbt/sysdep.h"

/**
 *  \brief Create a new setset data structure
 *  \param size The initial size of the setset (in number of elements)
 *  \return The created setset
 */
xbt_setset_t xbt_setset_new(unsigned int size)
{
  xbt_setset_elm_entry_t first_elm = NULL;
  xbt_setset_t setset = xbt_new0(s_xbt_setset_t, 1);
  setset->elm_array = xbt_dynar_new(sizeof(u_xbt_setset_elm_entry_t), NULL);
  setset->sets = xbt_fifo_new();
  /* Expand the elements dynar to the size indicated by the user, */
  /* then create the first element, get a pointer to it and add it to the */
  /* free elements list */
  xbt_dynar_shrink(setset->elm_array, size);  
  first_elm = (xbt_setset_elm_entry_t)xbt_dynar_push_ptr(setset->elm_array);
  first_elm->free.next = 0;
  return setset;
}

/**
 *  \brief Destroy a setset and free all it's resources
 *  \param The setset to destroy
 */
void xbt_setset_destroy(xbt_setset_t setset)
{
  xbt_dynar_free(&setset->elm_array);
  xbt_fifo_free(setset->sets);
  xbt_free(setset);
  /* FIXME: check if we should trash the stored objects */
}

/* Add an element to the setset, this will assign to it an index */
xbt_setset_elm_entry_t _xbt_setset_elm_add(xbt_setset_t setset, void *obj)
{
  xbt_setset_elm_entry_t new_entry = NULL;
  xbt_setset_elm_t e = (xbt_setset_elm_t)obj;
  xbt_assert0(e->ID == 0, "Adding element with non NULL ID");
  xbt_setset_elm_entry_t first_elm = 
    (xbt_setset_elm_entry_t)xbt_dynar_get_ptr(setset->elm_array, 0);
  
  /* Before create a new elm entry check if there is one in the free elm list.*/
  /* If there is not free elm entries, then create a new one  */
  if(first_elm->free.next != 0){
    e->ID = first_elm->free.next;
    new_entry = (xbt_setset_elm_entry_t)xbt_dynar_get_ptr(setset->elm_array, first_elm->free.next);
    first_elm->free.next = new_entry->free.next;
  }else{
    new_entry = (xbt_setset_elm_entry_t)xbt_dynar_push_ptr(setset->elm_array);
    e->ID = xbt_dynar_length(setset->elm_array) - 1;    
  }
  /* Initialize the new element data structure and add it to the hash */
  new_entry->info.refcount = 0;
  new_entry->info.obj = e;
  return new_entry;
}

/* Remove from the setset the object stored at idx */
void _xbt_setset_elm_remove(xbt_setset_t setset, unsigned long idx)
{
  xbt_setset_elm_entry_t e_entry = xbt_dynar_get_ptr(setset->elm_array, idx);
  xbt_setset_elm_entry_t first_free = NULL;
  
  /* Decrease the refcount and proceed only if it is 0 */
  if(--e_entry->info.refcount > 0)
    return;

  /* Erase object ID */
  e_entry->info.obj->ID = 0;
  
  /* Link the elm entry to the list of free ones */
  first_free = xbt_dynar_get_ptr(setset->elm_array, 0);
  e_entry->free.next = first_free->free.next;
  first_free->free.next = idx;
}

/* Get the object associated to a given index */
/* WARNING: it must be a valid index! */
void *_xbt_setset_idx_to_obj(xbt_setset_t setset, unsigned long idx)
{
  xbt_setset_elm_entry_t e_entry = xbt_dynar_get_ptr(setset->elm_array, idx);
  return e_entry->info.obj;
}

/* Increase the refcount of an element */
void _xbt_setset_elm_use(xbt_setset_t setset, unsigned long idx)
{
  xbt_setset_elm_entry_t e_entry = xbt_dynar_get_ptr(setset->elm_array, idx);
  e_entry->info.refcount++;  
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
  newset->elmcount = 0;
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
  xbt_setset_set_reset(set);
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
void xbt_setset_set_insert(xbt_setset_set_t set, void* obj)
{
  xbt_setset_elm_t e = (xbt_setset_elm_t)obj;

  if(e->ID == 0)
    _xbt_setset_elm_add(set->setset, e);
  
  /* Check if we need to expand the bitmap */
  if(set->size * BITS_INT - 1 < e->ID){
    set->bitmap = xbt_realloc(set->bitmap, (e->ID / BITS_INT + 1) * sizeof(int));
    memset(&set->bitmap[set->size], 0, ((e->ID / BITS_INT + 1) - set->size) * sizeof(int));
    set->size = (e->ID / BITS_INT + 1);
  }
  
  if(!_is_bit_set(e->ID % BITS_INT, set->bitmap[e->ID / BITS_INT])){
    set->elmcount++;
    _xbt_setset_elm_use(set->setset, e->ID);
    _set_bit(e->ID, set->bitmap);
  }

  return;
}

/**
 *  \brief Remove an element from a set
 *  \param set The set from which the element is going to be removed
 *  \param obj The element to remove
 */
void xbt_setset_set_remove(xbt_setset_set_t set, void* obj)
{
  xbt_setset_elm_t e = (xbt_setset_elm_t)obj;
  if(e->ID != 0){
    /* If the index of the object is between the bitmap then unset it, otherwise
       do not do anything, because we already know it is not in the set */
    if(set->size * BITS_INT >= e->ID){
      if(_is_bit_set(e->ID % BITS_INT, set->bitmap[e->ID / BITS_INT])){
        set->elmcount--;
        _unset_bit(e->ID, set->bitmap);
        _xbt_setset_elm_remove(set->setset, e->ID);
      }
    }
  }
  return;
}

/**
 *  \brief Remove all the elements of a set
 *  \param set The set to empty
 */
void xbt_setset_set_reset(xbt_setset_set_t set)
{
  int i,k;
  /* Traverse the set and remove every element */
  for(i=0; i < set->size; i++){
    if(set->bitmap[i] != 0){
      for(k=0; k < BITS_INT; k++){
        if(_is_bit_set(k,set->bitmap[i])){
          _xbt_setset_elm_remove(set->setset, i * BITS_INT + k);
        }
      }
    }
  }
  set->elmcount = 0;
}

/**
 *  \brief Choose one element of a set (but do not remove it)
 *  \param set The set
 *  \return An element that belongs to set \a set
 */
void *xbt_setset_set_choose(xbt_setset_set_t set)
{
  int i,k;
  /* Traverse the set and return the first element */
  for(i=0; i < set->size; i++){
    if(set->bitmap[i] != 0){
      for(k=0; k < BITS_INT; k++){
        if(_is_bit_set(k,set->bitmap[i])){
          return _xbt_setset_idx_to_obj(set->setset, i * BITS_INT + k);
        }
      }
    }
  }
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
  if(obj){
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
int xbt_setset_set_belongs(xbt_setset_set_t set, void* obj)
{
  xbt_setset_elm_t e = (xbt_setset_elm_t)obj;
  if(e->ID != 0 && e->ID <= set->size * BITS_INT){  
    return _is_bit_set(e->ID % BITS_INT, set->bitmap[e->ID / BITS_INT]);
  }
  return FALSE;
}

int xbt_setset_set_size(xbt_setset_set_t set)
{
  return set->elmcount;
}


/**
 *  \brief Add two sets
 *         Add two sets storing the result in the first one
 *  \param set1 The first set
 *  \param set2 The second set
 */
void xbt_setset_add(xbt_setset_set_t set1, xbt_setset_set_t set2)
{
  if(set1->size < set2->size){
    xbt_realloc(set1->bitmap, set2->size * sizeof(unsigned int));
    set1->size = set2->size;
  }

  int i,k;
  /* Traverse the second set and add every element that is not member of the
     first set to it */
  for(i=0; i < set2->size; i++){
    if(set2->bitmap[i] != 0){
      for(k=0; k < BITS_INT; k++){
        if(_is_bit_set(k,set2->bitmap[i]) && !_is_bit_set(k, set1->bitmap[i])){
          set1->elmcount++;
          _xbt_setset_elm_use(set1->setset, i * BITS_INT + k);
          _set_bit(i * BITS_INT + k, set1->bitmap);
        }
      }
    }
  }
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
  int i,k;
  /* Traverse the second set and remove every element that is member of it to
     the first set */
  for(i=0; i < min(set1->size,set2->size); i++){
    if(set2->bitmap[i] != 0){
      for(k=0; k < BITS_INT; k++){
        if(_is_bit_set(k,set2->bitmap[i]) && _is_bit_set(k, set1->bitmap[i])){
          set1->elmcount--;
          _xbt_setset_elm_remove(set1->setset, i * BITS_INT + k);
          _unset_bit(i * BITS_INT + k, set1->bitmap);
        }
      }
    }
  }
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
  int i,k;
  /* Traverse the first set and remove every element that is not member of the second set */
  for(i=0; i < set1->size; i++){
    if(set1->bitmap[i] != 0){
      for(k=0; k < BITS_INT; k++){
        if(_is_bit_set(k, set1->bitmap[i]) && 
            (i >= set2->size || !_is_bit_set(k,set2->bitmap[i]))){
          set1->elmcount--;
          _xbt_setset_elm_remove(set1->setset, i * BITS_INT + k);
          _unset_bit(i * BITS_INT + k, set1->bitmap);
        }
      }
    }
  }
  return;
}

/* Get the cursor to point to the first element of a set */
void xbt_setset_cursor_first(xbt_setset_set_t set, xbt_setset_cursor_t *cursor)
{
  (*cursor) = xbt_new0(s_xbt_setset_cursor_t, 1);
  (*cursor)->set = set;
  int i,k;
  /* Traverse the set and point the cursor to the first element */
  for(i=0; i < set->size; i++){
    if(set->bitmap[i] != 0){
      for(k=0; k < BITS_INT; k++){
        if(_is_bit_set(k,set->bitmap[i])){
          (*cursor)->idx = i * BITS_INT + k;
          return; 
        }
      }
    }
  }
}

/* Get the data pointed by a cursor */
int xbt_setset_cursor_get_data(xbt_setset_cursor_t cursor, void **data)
{
  if(cursor->idx == 0){
    xbt_free(cursor);
    *data = NULL;
    return FALSE;
  }else{
    *data = _xbt_setset_idx_to_obj(cursor->set->setset, cursor->idx);
    return TRUE;
  }
}

/* Advance a cursor to the next element */
void xbt_setset_cursor_next(xbt_setset_cursor_t cursor)
{
 int i,k;
  cursor->idx++;
  /* Traverse the set and point the cursor to the first element */
  for(i = cursor->idx / BITS_INT; i < cursor->set->size; i++){
    if(cursor->set->bitmap[i] != 0){
      for(k = cursor->idx % BITS_INT; k < BITS_INT; k++){
        if(_is_bit_set(k,cursor->set->bitmap[i])){
          cursor->idx = i * BITS_INT + k;
          return; 
        }
      }
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








