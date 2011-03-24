/*
 * lib.c
 *
 *  Created on: 16 mars 2011
 *      Author: navarrop
 */

#include <string.h>
#include <stdio.h>
#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/mallocator.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt_modinter.h"
#include "xbt/lib.h"

#define DJB2_HASH_FUNCTION
//#define FNV_HASH_FUNCTION

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_lib, xbt,
                                "Libraries provide the same functionalities than hash tables");

/*####[ Private prototypes ]#################################################*/
static void *lib_mallocator_new_f(void);
static void lib_mallocator_free_f(void *dict);
static void lib_mallocator_reset_f(void *dict);

static void xbt_lib_set_ext(xbt_lib_t lib,const char *key,
                            int key_len, int level, void *data);

static unsigned int xbt_lib_hash_ext(const char *str,
                                                 int str_len);
static unsigned int xbt_lib_hash(const char *str);
static void xbt_lib_rehash(xbt_lib_t lib);


static xbt_lib_cursor_t xbt_lib_cursor_new(const xbt_lib_t lib);
static void xbt_lib_cursor_rewind(xbt_lib_cursor_t cursor);
static void xbt_lib_cursor_free(xbt_lib_cursor_t * cursor);

/*####[ Code ]###############################################################*/

unsigned int xbt_lib_size(xbt_lib_t lib)
{
  return (lib ? (unsigned int) lib->count : (unsigned int) 0);
}

static xbt_libelm_t xbt_libelm_new(const char *key,
                              int key_len,
                              unsigned int hash_code,
                              void *content,int level, int level_numbers)
{
  xbt_libelm_t element = xbt_malloc0(sizeof (s_xbt_libelm_t) + sizeof(void *)*(level_numbers-1));

  element->key = xbt_new(char, key_len + 1);
  memcpy((void *) element->key, (void *) key, key_len);
  element->key[key_len] = '\0';

  element->key_len = key_len;
  element->hash_code = hash_code;

  (&(element->content))[level] = content;
  element->next = NULL;

  return element;
}

xbt_lib_t xbt_lib_new(void)
{
  xbt_lib_t lib;
  lib = xbt_new0(s_xbt_lib_t, 1);
  lib->table_size = 127;
  lib->table = xbt_new0(xbt_libelm_t, lib->table_size + 1);
  lib->count = 0;
  lib->fill = 0;
  lib->levels = 0;
  lib->free_f = NULL;

  return lib;
}

void xbt_lib_free(xbt_lib_t * lib)
{
  int i,j;
  xbt_libelm_t current, previous;
  int table_size;
  xbt_libelm_t *table;
  xbt_lib_t l = *lib;

  if (l) {
    table_size = l->table_size;
    table = l->table;

    for (i = 0; l->count && i <= table_size; i++) {
      current = table[i];
      while (current != NULL) {
			previous = current;
			current = current->next;
			xbt_free(previous->key);
			for(j=0; j< l->levels; j++)
			  if((&(previous->content))[j])
				  l->free_f[j]( (&(previous->content))[j]);
	    	xbt_free(previous);
			l->count--;
      }
    }
    xbt_free(table);
    xbt_free(l->free_f);
    xbt_free(l);
    l = NULL;
  }
}

int xbt_lib_add_level(xbt_lib_t lib, void_f_pvoid_t free_f)
{
	XBT_DEBUG("xbt_lib_add_level");

	lib->free_f = realloc(lib->free_f,sizeof(void_f_pvoid_t)*(lib->levels+1));
	lib->free_f[lib->levels]=free_f;
	return lib->levels++;
}

void xbt_lib_set(xbt_lib_t lib, const char *name, int level, void *obj)
{
	XBT_DEBUG("xbt_lib_set key '%s' with object '%p'",name,obj);
	xbt_lib_set_ext(lib, name, strlen(name),level, obj);
}

void *xbt_lib_get_or_null(xbt_lib_t lib, const char *key, int level)
{
	unsigned int hash_code = xbt_lib_hash(key);
	xbt_libelm_t current;

	if(!lib) xbt_die("Lib is NULL!");

	current = lib->table[hash_code & lib->table_size];
	while (current != NULL &&
		 (hash_code != current->hash_code || strcmp(key, current->key)))
	current = current->next;

	if (current == NULL)
	return NULL;

	return (&(current->content))[level];
}

static void *lib_mallocator_new_f(void)
{
  return xbt_new(s_xbt_lib_t, 1);
}

static void lib_mallocator_free_f(void *lib)
{
  xbt_free(lib);
}

static void lib_mallocator_reset_f(void *lib)
{
  /* nothing to do because all fields are
   * initialized in xbt_dict_new
   */
}

void xbt_lib_reset(xbt_lib_t *lib)
{
  XBT_DEBUG("xbt_lib_reset");
  int i,j;
  xbt_libelm_t current, next;
  xbt_lib_t l = *lib;
  int levels = l->levels;
  if(!l) xbt_die("Lib is NULL!");

  if (l->count == 0)
    return;

  for (i = 0; i <= l->table_size; i++) {
    current = l->table[i];
    while (current) {
    	next = current->next;
    	xbt_free(current->key);
    	for(j=0; j< levels; j++)
    		if((&(current->content))[j] && (l->free_f[j]))
    			l->free_f[j]((&(current->content))[j]);
    	xbt_free(current);
    	current = next;
    }
    l->table[i] = NULL;
  }
  xbt_free(l->free_f);
  l->count = 0;
  l->fill = 0;
  l->levels = 0;
  l->free_f = NULL;
}

int xbt_lib_length(xbt_lib_t lib)
{
  if(!lib) xbt_die("Lib is NULL!");
  return lib->count;
}

static void xbt_lib_set_ext(xbt_lib_t lib,
                            const char *key,
                            int key_len,
                            int level,
                            void *data)
{

  unsigned int hash_code = xbt_lib_hash_ext(key, key_len);

  xbt_libelm_t current, previous = NULL;
  if(!lib) xbt_die("Lib is NULL!");

  XBT_DEBUG("ADD '%.*s' hash = '%d', size = '%d', & = '%d' to level : %d",
		  key_len,
		  key,
		  hash_code,
          lib->table_size,
          hash_code & lib->table_size,
          level);

  current = lib->table[hash_code & lib->table_size];
  while (current != NULL &&
         (hash_code != current->hash_code || key_len != current->key_len
          || memcmp(key, current->key, key_len))) {
    previous = current;
    current = current->next;
  }

  if (current == NULL) {/* this key doesn't exist yet */
		current = xbt_libelm_new(key, key_len, hash_code, data,level,lib->levels);
		lib->count++;
		if (previous == NULL) {
		lib->table[hash_code & lib->table_size] = current;
		lib->fill++;
		if ((lib->fill * 100) / (lib->table_size + 1) > MAX_FILL_PERCENT)
		  xbt_lib_rehash(lib);
		} else {
		previous->next = current;
		}
  }
  else
	  if( (&(current->content))[level] == NULL )/* this key exist but level is empty */
	  {
		  (&(current->content))[level] = data;
	  }
	  else
	  {/* there is already an element with the same key: overwrite it */
			XBT_DEBUG("Replace %.*s by %.*s under key %.*s",
				   key_len, (char *) (&(current->content))[level],
				   key_len, (char *) data, key_len, (char *) key);
			if (current->content != NULL) {
			  free((&(current->content))[level]);
			}
			(&(current->content))[level] = data;
	  }
}

/**
 * Returns the hash code of a string.
 */
static unsigned int xbt_lib_hash_ext(const char *str,
                                                 int str_len)
{


#ifdef DJB2_HASH_FUNCTION
  /* fast implementation of djb2 algorithm */
  int c;
  register unsigned int hash = 5381;

  while (str_len--) {
    c = *str++;
    hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
  }
# elif defined(FNV_HASH_FUNCTION)
  register unsigned int hash = 0x811c9dc5;
  unsigned char *bp = (unsigned char *) str;    /* start of buffer */
  unsigned char *be = bp + str_len;     /* beyond end of buffer */

  while (bp < be) {
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) +
        (hash << 24);

    /* xor the bottom with the current octet */
    hash ^= (unsigned int) *bp++;
  }

# else
  register unsigned int hash = 0;

  while (str_len--) {
    hash += (*str) * (*str);
    str++;
  }
#endif

  return hash;
}

static unsigned int xbt_lib_hash(const char *str)
{
#ifdef DJB2_HASH_FUNCTION
  /* fast implementation of djb2 algorithm */
  int c;
  register unsigned int hash = 5381;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
  }

# elif defined(FNV_HASH_FUNCTION)
  register unsigned int hash = 0x811c9dc5;

  while (*str) {
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) +
        (hash << 24);

    /* xor the bottom with the current byte */
    hash ^= (unsigned int) *str++;
  }

# else
  register unsigned int hash = 0;

  while (*str) {
    hash += (*str) * (*str);
    str++;
  }
#endif
  return hash;
}

static void xbt_lib_rehash(xbt_lib_t lib)
{
	const int oldsize = lib->table_size + 1;
	register int newsize = oldsize * 2;
	register int i;
	register xbt_libelm_t *currcell;
	register xbt_libelm_t *twincell;
	register xbt_libelm_t bucklet;
	register xbt_libelm_t *pprev;

	currcell =
	  (xbt_libelm_t *) xbt_realloc((char *) lib->table,
									newsize * (sizeof(xbt_libelm_t) + sizeof(void*)*(lib->levels - 1)));
	memset(&currcell[oldsize], 0, oldsize * (sizeof(xbt_libelm_t) + sizeof(void*)*(lib->levels - 1)));       /* zero second half */
	lib->table_size = --newsize;
	lib->table = currcell;
	XBT_DEBUG("REHASH (%d->%d)", oldsize, newsize);

	for (i = 0; i < oldsize; i++, currcell++) {
	if (!*currcell)             /* empty cell */
	  continue;
	twincell = currcell + oldsize;
	for (pprev = currcell, bucklet = *currcell; bucklet; bucklet = *pprev) {
	  /* Since we use "& size" instead of "%size" and since the size was doubled,
		 each bucklet of this cell must either :
		 - stay  in  cell i (ie, currcell)
		 - go to the cell i+oldsize (ie, twincell) */
	  if ((bucklet->hash_code & newsize) != i) {        /* Move to b */
		*pprev = bucklet->next;
		bucklet->next = *twincell;
		if (!*twincell)
		  lib->fill++;
		*twincell = bucklet;
		continue;
	  } else {
		pprev = &bucklet->next;
	  }

	}

	if (!*currcell)             /* everything moved */
	  lib->fill--;
	}

}

void xbt_lib_cursor_first(const xbt_lib_t lib,
                                      xbt_lib_cursor_t * cursor)
{
  XBT_DEBUG("xbt_lib_cursor_first");
  if (!*cursor) {
    XBT_DEBUG("Create the cursor on first use");
    *cursor = xbt_lib_cursor_new(lib);
  } else {
    xbt_lib_cursor_rewind(*cursor);
  }
  if (lib != NULL && (*cursor)->current == NULL) {
    xbt_lib_cursor_step(*cursor);      /* find the first element */
  }
}

static xbt_lib_cursor_t xbt_lib_cursor_new(const xbt_lib_t lib)
{
  xbt_lib_cursor_t res = NULL;

  res = xbt_new(s_xbt_lib_cursor_t, 1);
  res->lib = lib;

  xbt_lib_cursor_rewind(res);

  return res;
}

static void xbt_lib_cursor_rewind(xbt_lib_cursor_t cursor)
{
  XBT_DEBUG("xbt_lib_cursor_rewind");
  xbt_assert(cursor);

  cursor->line = 0;
  if (cursor->lib != NULL) {
    cursor->current = cursor->lib->table[0];
  } else {
    cursor->current = NULL;
  }
}

int xbt_lib_cursor_get_or_free(xbt_lib_cursor_t * cursor,
                                           char **key, void **data)
{

  xbt_libelm_t current;

  XBT_DEBUG("xbt_lib_get_or_free");

  if (!cursor || !(*cursor))
    return FALSE;

  current = (*cursor)->current;
  if (current == NULL) {        /* no data left */
    xbt_lib_cursor_free(cursor);
    return FALSE;
  }

  *key = current->key;
  *data = &(current->content);
  return TRUE;
}

static void xbt_lib_cursor_free(xbt_lib_cursor_t * cursor)
{
  if (*cursor) {
    xbt_free(*cursor);
    *cursor = NULL;
  }
}

void xbt_lib_cursor_step(xbt_lib_cursor_t cursor)
{
  xbt_libelm_t current;
  int line;

  XBT_DEBUG("xbt_lib_cursor_step");
  xbt_assert(cursor);

  current = cursor->current;
  line = cursor->line;

  if (cursor->lib != NULL) {

    if (current != NULL) {
      XBT_DEBUG("current is not null, take the next element");
      current = current->next;
      XBT_DEBUG("next element: %p", current);
    }

    while (current == NULL && ++line <= cursor->lib->table_size) {
      XBT_DEBUG("current is NULL, take the next line");
      current = cursor->lib->table[line];
      XBT_DEBUG("element in the next line: %p", current);
    }
    XBT_DEBUG("search finished, current = %p, line = %d", current, line);

    cursor->current = current;
    cursor->line = line;
  }
}
