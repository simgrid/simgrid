/* $Id$ */

/* dict - a generic dictionnary, variation over the B-tree concept          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras_private.h"
#include "dict_private.h"  /* prototypes of this module */

#include <stdlib.h> /* malloc() */
#include <string.h> /* strlen() */

#include <stdio.h>

GRAS_LOG_EXTERNAL_CATEGORY(dict);
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(dict_elm,dict);

GRAS_LOG_NEW_SUBCATEGORY(dict_add,dict);
GRAS_LOG_NEW_SUBCATEGORY(dict_search,dict);
GRAS_LOG_NEW_SUBCATEGORY(dict_remove,dict);
GRAS_LOG_NEW_SUBCATEGORY(dict_collapse,dict);
GRAS_LOG_NEW_SUBCATEGORY(dict_multi,dict);

/*####[ Private prototypes ]#################################################*/

static inline gras_error_t _gras_dictelm_alloc(char                *key,
					       int                  offset,
					       int                  key_len,
					       void                *data,
					       void_f_pvoid_t      *free_ctn,
					       /*OUT*/gras_dictelm_t **where);
static void         _dictelm_wrapper_free(void*);

static inline void  _str_prefix_lgr(const char *key1,
				    int         key_len1,
				    const char *key2,
				    int         key_len2,
				    int        *offset,
				    int        *match);


static gras_error_t _gras_dictelm_dump_rec(gras_dictelm_t *head,
					   int             offset,
					   void_f_pvoid_t *output);



static gras_error_t _gras_dictelm_set_rec(gras_dictelm_t *head,
					     char           *key,
					     int             key_len,
					     int             offset,
					     void           *data,
					     void_f_pvoid_t *free_ctn);
static gras_error_t _gras_dictelm_get_rec(gras_dictelm_t *head,
					       const char     *key,
					       int             key_len,
					       int             offset,
					       /* OUT */void **data);
static gras_error_t _gras_dictelm_remove_rec(gras_dictelm_t *head,
					     const char     *key,
					     int             key_len,
					     int             offset);

static inline
void
_collapse_if_need(gras_dictelm_t *p_head,
		  int             pos,
		  int             offset);

/* ---- */

static inline
void *
memdup(const void * const ptr,
       const size_t       length) {
  void * new_ptr = NULL;

  new_ptr = malloc(length);

  if (new_ptr) {
    memcpy(new_ptr, ptr, length);
  }

  return new_ptr;
}

/*
 * _gras_nibble_to_char:
 *
 * Change any byte to a printable char
 */

static inline
char
_gras_nibble_to_char(unsigned char c) {
  c &= 0x0f;
  return c>9 ? c-10+'a' : c + '0';
}

/*
 * _gras_bytes_to_string:
 *
 * Change any byte array to a printable string
 * The length of string_container should at least be data_len*2+1 
 */
static inline
char *
_gras_bytes_to_string(char * const ptr,
                      int          data_len,
                      char * const string_container) {
  unsigned char *src = (unsigned char *)ptr;
           char *dst = string_container;

  while (data_len--) {
    *dst++ = _gras_nibble_to_char(*src   & 0x0f     );
    *dst++ = _gras_nibble_to_char(*src++ & 0xf0 >> 4);
  }

  *dst = 0;

  return ptr;
}

/* ---- */

/*
 * _gras_dictelm_alloc:
 *
 * Alloc a dict element with no child.
 */
static 
inline
gras_error_t
_gras_dictelm_alloc(char                *key,
		    int                  key_len,
		    int                  offset,
		    void                *data,
		    void_f_pvoid_t      *free_ctn,
                 /*OUT*/gras_dictelm_t **pp_elm) {
  gras_error_t   errcode = no_error;
  gras_dictelm_t *p_elm  = NULL;

  if (!(p_elm = calloc(1, sizeof(gras_dictelm_t)))) {
    if (free_ctn && data) {
      free_ctn(data);
    }

    RAISE_MALLOC;
  }

  p_elm->key      = key;
  p_elm->key_len  = key_len;
  p_elm->offset   = offset;
  p_elm->content  = data;
  p_elm->free_ctn = free_ctn;

  errcode = gras_dynar_new(&(p_elm->sub), sizeof(gras_dictelm_t*), 
			   _dictelm_wrapper_free);
  if (errcode != no_error) {
    if (free_ctn && data) {
      free_ctn(data);
    }
    free(p_elm);
    return errcode;
  }

  *pp_elm = p_elm;

  return errcode;
}

/**
 * gras_dictelm_free:
 *
 * @pp_elm: the dict elem to be freed
 *
 * Frees a dictionnary element with all its childs.
 */
void
gras_dictelm_free(gras_dictelm_t **pp_elm)  {
  if (*pp_elm) {
    gras_dictelm_t *p_elm = *pp_elm;

    gras_dynar_free(p_elm->sub);

    if (p_elm->key) {
      free(p_elm->key);
    }

    if (p_elm->free_ctn && p_elm->content) {
      p_elm->free_ctn(p_elm->content);
    }

    memset(p_elm, 0, sizeof (*p_elm));

    free(p_elm);
    *pp_elm = NULL;
  }
}

/**
 * _dictelm_wrapper_free:
 *
 * a wrapper to free dictelm with the right prototype to be usable within dynar
 */
static
void
_dictelm_wrapper_free(void *pp_elm) {
  DEBUG2("Free dictelm %p (*=%p)", pp_elm, *(void**)pp_elm);
  gras_dictelm_free((gras_dictelm_t**)pp_elm);
}

/*####[ utility functions ]##################################################*/
/**
 * _str_prefix_lgr:
 *
 *
 * Returns the length of the common prefix of @str1 and @str2.
 * Do make sure the strings are not null
 */
static inline
void
_str_prefix_lgr(const char *key1,
		int         key_len1,
		const char *key2,
		int         key_len2,
		int        *p_offset,
		int        *p_match) {
  const int old_offset = *p_offset;
  int       o          = *p_offset;
  int       m          = *p_match;

  m = 0;

  /*CDEBUG3(dict_search, "%s: [%s] <=> [%s]", __FUNCTION__, s1, s2);*/

  if (o < key_len1  &&  o < key_len2) {

    while (key1[o] == key2[o]) {
      o++;

      if (!(o < key_len1  &&  o < key_len2))
        break;

    }

  }


  if (o != old_offset) {

    if (o >= key_len1) {

      if (o >= key_len2) {
        m = 1;
      } else {
        m = 2;
      }

    } else if (o >= key_len2) {
      m = 3;
    } else {
      m = 4;
    }
  }


  *p_offset = o;
  *p_match  = m;
}

/**
 * _dictelm_child_cmp:
 *
 * Compares two dictelm keys and return their matching (using the same 
 * convention than @_gras_dict_child_search() )
 */
static inline
void
_dict_child_cmp(gras_dictelm_t *p_dict,
                int          pos,
                const char  *key,
                const int    key_len,
                int         *p_offset,
                int         *p_match,
                int         *p_cmp) {
  gras_dictelm_t  *p_child = NULL;
  int           cmp     = 0;
  int           o       = *p_offset;
  int           m       = *p_match;

  gras_dynar_get(p_dict->sub, pos, &p_child);

  /* Compute the length of the prefix
     and if the searched key is before or after cur */
  _str_prefix_lgr(p_child->key, p_child->key_len,
		  key,          key_len,
		  &o, &m);


  if (m) /* found, get out */
    goto end;

  if (o < p_child->key_len  &&  (o >= key_len  ||  key[o] < p_child->key[o])) {
    cmp = -1;
  } else {
    cmp =  1;
  }

  CDEBUG3(dict_search, "Cmp %s and %s => %d", p_child->key + *p_offset,
	  key + *p_offset, cmp);

 end:
  *p_offset = o;
  *p_match  = m;
  *p_cmp    = cmp;
}

/**
 * _gras_dict_child_search:
 *
 * Search where would be inserted @key between the childs of @p_elm.
 * 
 * Returns position of the child having a common prefix with this key        
 * If *match==0, no child have a common prefix                               
 *               *pos is where to add the key                                
 * If *match==1, A child (located at *pos) have exactly this key             
 * If *match==2, A child (located at *pos) constitutes a prefix of the key   
 *               the recursion have to go on that guy                        
 *               *prefix = the size of the key eaten at this level           
 * If *match==3  The key is a prefix of the child at *pos                    
 * If *match==4, A child (loc. at *pos) share a common prefix with this key  
 *               *prefix = size of the prefix.                               
 *               If searching, that's a mismatch.                            
 *               If inserting, you have to break the child and create an     
 *                 internal node having {child, key} as childs               
 * offset is used in input and output. In input, that's the length of the key
 *  handled by previous levels of recursion. In output, that the one counting
 *  also this level.                                                         
 */
static inline
void
_gras_dictelm_child_search(gras_dictelm_t *p_elm,
			   const char  *key,
			   int          key_len,
			   int         *p_pos,
			   int         *p_offset,
			   int         *p_match) {

  int          p       = 0;
  int          o       = *p_offset;
  int          m       = 0;
  int          len     = 0;

  
  CDEBUG3(dict_search, "search child [%s] under [%s] (len=%d)",
	  key,
          p_elm?p_elm->key:"(head)",
  	  (p_elm&&p_elm->sub)?gras_dynar_length(p_elm->sub):0);
  

  len = gras_dynar_length(p_elm->sub);

  for (p = 0; p < len; p++) {
    int          cmp     = 0;

    _dict_child_cmp(p_elm, p, key, key_len, &o, &m, &cmp);

    if (m)
      break;

    o = *p_offset;
    m = 0;
  }

  *p_offset = o;
  *p_pos    = p;
  *p_match  = m;
  CDEBUG3(dict_search, "search [%s] in [%s] => %s",
	  key,
          p_elm?p_elm->key:"(head)",
	  ( m == 0 ? "no child have a common prefix" :
	    ( m == 1 ? "selected child have exactly this key" :
	      ( m == 2 ? "selected child constitutes a prefix" :
		( m == 3 ? "key is a prefix of selected child" :
		  (m == 4 ? "selected child share a prefix" :
		   "internal error")))))
	  );  
}

/**
 * _gras_dictelm_change_value:
 *
 * Change the value of the dictelm, making sure to free the old one, if any.
 */
static inline
void
_gras_dictelm_change_value(gras_dictelm_t    *p_elm,
			   void           *data,
			   void_f_pvoid_t *free_ctn) {

  if (p_elm->content && p_elm->free_ctn) {
    p_elm->free_ctn(p_elm->content);
  }

  p_elm->free_ctn = free_ctn;
  p_elm->content  = data;
}

/**
 * _gras_dictelm_set_rec:
 *
 * @head: the head of the dict
 * @key: the key to set the new data
 * @offset: offset on key.
 * @data: the data to add in the dict
 * @Returns: a gras_error
 *
 * set the @data in the structure under the @key. The @key is destroyed
 * in the process. Think to strdup it before.
 *
 * This is a helper function to gras_dict_set which locks the struct and
 * strdup the key before action. 
 */
gras_error_t
_gras_dictelm_set_rec(gras_dictelm_t     *p_head,
			 char            *key,
			 int              key_len,
			 int              offset,
			 void            *data,
			 void_f_pvoid_t  *free_ctn) {
  gras_error_t errcode    = no_error;
  int          match      = 0;
  int          pos        = 0;
  const int    old_offset = offset;

  CDEBUG4(dict_add, "--> Insert '%s' after '%s' (offset=%d) in tree %p",
	  key, ((p_head && p_head->key) ? p_head->key : "(head)"), offset,
	  p_head);

  /*** The trivial cases first ***/

  /* there is no key (we did enough recursion), change the value of head */
  if (offset >= key_len) {

    CDEBUG0(dict_add, "--> Change the value of head");

    _gras_dictelm_change_value(p_head, data, free_ctn);
    free(key); /* Keep the key used in the tree */

    return errcode;
  }

  /*** Search where to add this child, and how ***/
  _gras_dictelm_child_search(p_head, key, key_len, &pos, &offset, &match);

  CDEBUG3(dict_add, "child_search => pos=%d, offset=%d, match=%d",
	  pos, offset, match);

  switch (match) {

  case 0: /* no child have a common prefix */
    {
      gras_dictelm_t *p_child = NULL;

      TRY(_gras_dictelm_alloc(key, key_len, offset, data, free_ctn, &p_child));
      CDEBUG1(dict_add, "-> Add a child %p", p_child);
      TRY(gras_dynar_insert_at(p_head->sub, pos, &p_child));

      return errcode;
    }

  case 1: /* A child have exactly this key => change its value*/
    {
      gras_dictelm_t *p_child = NULL;

      gras_dynar_get(p_head->sub, pos, &p_child);
      CDEBUG1(dict_add, "-> Change the value of the child %p", p_child);
      _gras_dictelm_change_value(p_child, data, free_ctn);

      free(key);

      return errcode;
    }

  case 2: /* A child constitutes a prefix of the key => recurse */
    {
      gras_dictelm_t *p_child = NULL;

      gras_dynar_get(p_head->sub, pos, &p_child);
      CDEBUG2(dict_add,"-> Recurse on %p (offset=%d)", p_child, offset);

      return _gras_dictelm_set_rec(p_child, key, key_len, 
				      offset, data, free_ctn);
    }

  case 3: /* The key is a prefix of the child => child becomes child of p_new */
    {
      gras_dictelm_t *p_new   = NULL;
      gras_dictelm_t *p_child = NULL;

      gras_dynar_get(p_head->sub, pos, &p_child);
      TRY(_gras_dictelm_alloc(key, key_len, old_offset, data, free_ctn, &p_new));

      CDEBUG2(dict_add, "-> The child %p become child of new dict (%p)",
              p_child, p_new);

      TRY(gras_dynar_push(p_new->sub, &p_child));
      p_child->offset = offset;
      TRY(gras_dynar_set(p_head->sub, pos, &p_new));

      return errcode;
    }

  case 4: /* A child share a common prefix with this key => Common ancestor */
    {
      gras_dictelm_t *p_new       = NULL;
      gras_dictelm_t *p_child     = NULL;
      gras_dictelm_t *p_anc       = NULL;
      char        *anc_key     = NULL;
      int          anc_key_len = offset;

      TRY(_gras_dictelm_alloc(key, key_len, offset, data, free_ctn, &p_new));
      gras_dynar_get(p_head->sub, pos, &p_child);

      anc_key = memdup(key, anc_key_len);

      TRY(_gras_dictelm_alloc(anc_key, anc_key_len, old_offset, 
			      NULL, NULL, &p_anc));

      CDEBUG2(dict_add, "-> Make a common ancestor %p (%s)", p_anc, anc_key);

      if (key[offset] < p_child->key[offset]) {
        TRY(gras_dynar_push(p_anc->sub, &p_new));
        TRY(gras_dynar_push(p_anc->sub, &p_child));
      } else {
        TRY(gras_dynar_push(p_anc->sub, &p_child));
        TRY(gras_dynar_push(p_anc->sub, &p_new));
      }

      p_child->offset = offset;

      TRY(gras_dynar_set(p_head->sub, pos, &p_anc));

      return errcode;
    }

  default:
    RAISE_IMPOSSIBLE;
  }

}

/**
 * gras_dictelm_set_ext:
 *
 * @head: the head of the dict
 * @key: the key to set the new data
 * @data: the data to add in the dict
 * @Returns: a gras_error
 *
 * set the @data in the structure under the @key, which can be any kind 
 * of data, as long as its length is provided in @key_len.
 */
gras_error_t
gras_dictelm_set_ext(gras_dictelm_t **pp_head,
			const char      *_key,
			int              key_len,
			void            *data,
			void_f_pvoid_t  *free_ctn) {
  gras_error_t  errcode =  no_error;
  gras_dictelm_t  *p_head  = *pp_head;
  char         *key     =  NULL;

  key = memdup(_key, key_len);
  if (!key) 
    RAISE_MALLOC;

  /* there is no head, create it */
  if (!p_head) {
    gras_dictelm_t *p_child = NULL;

    CDEBUG0(dict_add, "Create an head");

    /* The head is priviledged by being the only one with a NULL key */
    TRY(_gras_dictelm_alloc(NULL, 0, 0, NULL, NULL, &p_head));

    TRY(_gras_dictelm_alloc(key, key_len, 0, data, free_ctn, &p_child));
    TRY(gras_dynar_insert_at(p_head->sub, 0, &p_child));

    *pp_head = p_head;

    return errcode;
  }

  return _gras_dictelm_set_rec(p_head, key, key_len, 0, data, free_ctn);
}

/**
 * gras_dictelm_set:
 *
 * @head: the head of the dict
 * @key: the key to set the new data
 * @data: the data to add in the dict
 * @Returns: a gras_error
 *
 * set the @data in the structure under the @key, which is a 
 * null terminated string.
 */
gras_error_t
gras_dictelm_set(gras_dictelm_t **pp_head,
		    const char      *_key,
		    void            *data,
		    void_f_pvoid_t  *free_ctn) {

  return gras_dictelm_set_ext(pp_head, _key, 1+strlen(_key), data, free_ctn);
}

/**
 * _gras_dict_get_rec:
 *
 * @head: the head of the dict
 * @key: the key to find data
 * @offset: offset on the key
 * @data: the data that we are looking for
 * @Returns: gras_error
 *
 * Search the given @key. mismatch_error when not found.
 */
static 
gras_error_t
_gras_dictelm_get_rec(gras_dictelm_t *p_head,
			   const char     *key,
			   int             key_len,
			   int             offset,
			   /* OUT */void **data) {

  gras_error_t errcode = no_error;

  CDEBUG2(dict_search, "Search %s in %p", key, p_head); 

  /*** The trivial case first ***/

  /* we did enough recursion, we're done */
  if (offset >= key_len) {
    *data = p_head->content;

    return errcode;
  }

  {
    int match = 0;
    int pos   = 0;

    *data = NULL; // Make it ready to answer 'not found' in one operation

    /*** Search where is the good child, and how good it is ***/
    _gras_dictelm_child_search(p_head, key, key_len, &pos, &offset, &match);

    switch (match) {

    case 0: /* no child have a common prefix */
      return mismatch_error;

    case 1: /* A child have exactly this key => Got it */
      {
        gras_dictelm_t *p_child = NULL;

        gras_dynar_get(p_head->sub, pos, &p_child);
        *data = p_child->content;

        return errcode;
      }

    case 2: /* A child constitutes a prefix of the key => recurse */
      {
        gras_dictelm_t *p_child = NULL;

        gras_dynar_get(p_head->sub, pos, &p_child);

        return _gras_dictelm_get_rec(p_child, key, key_len, offset, data);
      }

    case 3: /* The key is a prefix of the child => not found */
      return mismatch_error;

    case 4: /* A child share a common prefix with this key => not found */
      return mismatch_error;

    default:
      RAISE_IMPOSSIBLE;
    }
  }
}

/**
 * gras_dictelm_get_ext:
 *
 * @head: the head of the dict
 * @key: the key to find data
 * @data: the data that we are looking for
 * @Returns: gras_error
 *
 * Search the given @key. mismatch_error when not found.
 */
gras_error_t
gras_dictelm_get_ext(gras_dictelm_t *p_head,
			  const char     *key,
			  int             key_len,
			  /* OUT */void **data) {
  /* there is no head, go to hell */
  if (!p_head) {
    return mismatch_error;
  }

  return _gras_dictelm_get_rec(p_head, key, key_len, 0, data);
}

/**
 * gras_dictelm_get:
 *
 * @head: the head of the dict
 * @key: the key to find data
 * @data: the data that we are looking for
 * @Returns: gras_error
 *
 * Search the given @key. mismatch_error when not found.
 */
gras_error_t
gras_dictelm_get(gras_dictelm_t    *p_head,
                   const char     *key,
                   /* OUT */void **data) {

  return gras_dictelm_get_ext(p_head, key, 1+strlen(key), data);
}

/*----[ _gras_dict_collapse ]------------------------------------------------*/
static inline
void
_collapse_if_need(gras_dictelm_t *p_head,
		  int             pos,
		  int             offset) {
  gras_dictelm_t  *p_child = NULL;

  CDEBUG2(dict_collapse, "Collapse %d of %p... ", pos, p_head); fflush(stdout);

  if (pos >= 0) {
    /* Remove the child if |it's key| == 0 (meaning it's dead) */
    gras_dynar_get(p_head->sub, pos, &p_child);

    if (offset >= p_child->key_len) {

      gras_assert0(gras_dynar_length(p_child->sub) == 0,
		   "Found a dead child with grand childs. Internal error");

      CDEBUG1(dict_collapse, "Remove dead child %p.... ", p_child);
      gras_dynar_remove_at(p_head->sub, pos, &p_child);
    }
  }

  if (!p_head->key) {
    CDEBUG0(dict_collapse, "Do not collapse the head, you stupid programm");
    return;
  }

  if (p_head->content || p_head->free_ctn ||
      gras_dynar_length(p_head->sub) != 1) {
    CDEBUG0(dict_collapse, "Cannot collapse");
    return; /* cannot collapse */
  }

  gras_dynar_get(p_head->sub, 0, &p_child);

  /* Get the child's key as new key */
  CDEBUG1(dict_collapse, "Do collapse with only child %s", p_child->key); 

  p_head->content  = p_child->content;
  p_head->free_ctn = p_child->free_ctn;
  free(p_head->key);
  p_head->key      = p_child->key;
  p_head->key_len  = p_child->key_len;

  gras_dynar_free_container(p_head->sub) ;

  p_head->sub = p_child->sub;
  free(p_child);
}

/**
 * _gras_dict_remove_rec:
 *
 * @head: the head of the dict
 * @key: the key of the data to be removed
 * @offset: offset on the key
 * @Returns: gras_error_t
 *
 * Remove the entry associated with the given @key
 */
gras_error_t
_gras_dictelm_remove_rec(gras_dictelm_t *p_head,
			 const char  *key,
			 int          key_len,
			 int          offset) {
  gras_error_t errcode = no_error;

  /* there is no key to search, we did enough recursion => kill current */
  if (offset >= key_len) {
    int killme = 0; /* do I need to suicide me ? */

    if (p_head->content && p_head->free_ctn) {
      p_head->free_ctn(p_head->content);
    }

    killme = !gras_dynar_length(p_head->sub);
    p_head->content  = NULL;
    p_head->free_ctn = NULL;
    _collapse_if_need(p_head, -1, offset);

    if (killme) {
      DEBUG0("kill this node");
      p_head->key_len = 0; /* killme. Cleanup done one step higher in recursion */
    }

    return errcode;

  } else {
    int match      =      0;
    int pos        =      0;
    int old_offset = offset;

    /*** Search where is the good child, and how good it is ***/
    _gras_dictelm_child_search(p_head, key, key_len, &pos, &offset, &match);

    switch (match) {

    case 1: /* A child have exactly this key           => recurse */
    case 2: /* A child constitutes a prefix of the key => recurse */

      {
        gras_dictelm_t *p_child = NULL;

        gras_dynar_get(p_head->sub, pos, &p_child);
        /*DEBUG4("Recurse on child %d of %p to remove %s (prefix=%d)",
          pos, p_child, key+offset, offset);*/
        TRY(_gras_dictelm_remove_rec(p_child, key, key_len, offset));

        _collapse_if_need(p_head, pos, old_offset);
	return no_error;
      }


    case 0: /* no child have a common prefix */
    case 3: /* The key is a prefix of the child => not found */
    case 4: /* A child share a common prefix with this key => not found */

      return mismatch_error;


    default:
      RAISE_IMPOSSIBLE;

    }
  }
}

/**
 * gras_dictelm_remove_ext:
 *
 * @head: the head of the dict
 * @key: the key of the data to be removed
 * @Returns: gras_error_t
 *
 * Remove the entry associated with the given @key
 */
gras_error_t
gras_dictelm_remove_ext(gras_dictelm_t *p_head,
                     const char  *key,
                     int          key_len) {
  /* there is no head, go to hell */
  if (!p_head) {
    RAISE0(mismatch_error, "there is no head, go to hell");
  }
  
  return _gras_dictelm_remove_rec(p_head, key, key_len, 0);
}

/**
 * gras_dictelm_remove:
 *
 * @head: the head of the dict
 * @key: the key of the data to be removed
 * @Returns: gras_error_t
 *
 * Remove the entry associated with the given @key
 */
gras_error_t
gras_dictelm_remove(gras_dictelm_t *p_head,
		    const char     *key) {
  return _gras_dictelm_remove_rec(p_head, key, 1+strlen(key),0);
}

/*----[ _gras_dict_dump_rec ]------------------------------------------------*/
/* private function to do the job of gras_dict_dump recursively              */
/*---------------------------------------------------------------------------*/
static
gras_error_t
_gras_dictelm_dump_rec(gras_dictelm_t *p_head,
		       int             offset,
		       void_f_pvoid_t *output) {
  gras_error_t  errcode = no_error;
  gras_dictelm_t  *p_child =     NULL;
  char         *key     =     NULL;
  int           key_len =        0;
  int           i       =        0;

  if (!p_head)
    return no_error;

  printf("[%p] ", p_head);

  key     = p_head->key;
  key_len = p_head->key_len;

  if (key_len) {
    printf ("  ");
  }

  for (i = 0; i < offset; i++) {
    printf("-");
  }

  fflush(stdout);

  if (key) {

    if (!key_len) {
      printf ("HEAD");
    } else {
      char *key_string = NULL;

      key_string = malloc(key_len*2+1);
      if (!key_string)
        RAISE_MALLOC;

      _gras_bytes_to_string(key, key_len, key_string);

      printf("%s|(%d)", key_string + offset, offset);

      free(key_string);
    }

  }

  printf(" -> ");

  if (p_head->content) {

    if (output) {
      output(p_head->content);
    } else {
      printf("(data)");
    }

  } else {
    printf("(null)");
  }

  printf("    \t\t\t[ %d child(s) ]\n", gras_dynar_length(p_head->sub));

  gras_dynar_foreach(p_head->sub, i, p_child) 
    TRY(_gras_dictelm_dump_rec(p_child, p_child->offset, output));

  return errcode;
}

/**
 * gras_dictelm_dump:
 *
 * @head: the head of the dict
 * @output: a function to dump each data in the tree
 * @Returns: gras_error_t
 *
 * Ouputs the content of the structure. (for debuging purpose). @ouput is a
 * function to output the data. If NULL, data won't be displayed.
 */

gras_error_t
gras_dictelm_dump(gras_dictelm_t *p_head,
               void_f_pvoid_t *output) {
  return _gras_dictelm_dump_rec(p_head, 0, output);
}

/**
 * gras_dictelm_print_fct:
 *
 * @data:
 *
 * To dump multidict, this function dumps a dict
 */

void
gras_dictelm_print_fct(void *data) {
  printf("tree %p", data);
}

