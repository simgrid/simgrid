/* $Id$ */

/* dict - a generic dictionnary, variation over the B-tree concept          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "dict_private.h"  /* prototypes of this module */

XBT_LOG_EXTERNAL_CATEGORY(dict);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dict_elm,dict,"Dictionaries internals");

XBT_LOG_NEW_SUBCATEGORY(dict_add,dict,"Dictionaries internals: elements addition");
XBT_LOG_NEW_SUBCATEGORY(dict_search,dict,"Dictionaries internals: searching");
XBT_LOG_NEW_SUBCATEGORY(dict_remove,dict,"Dictionaries internals: elements removal");
XBT_LOG_NEW_SUBCATEGORY(dict_collapse,dict,"Dictionaries internals: post-removal cleanup");
XBT_LOG_NEW_SUBCATEGORY(dict_multi,dict,"Dictionaries internals: dictionaries of dictionaries");

/*####[ Private prototypes ]#################################################*/

static _XBT_INLINE void _xbt_dictelm_alloc(char                *key,
					     int                  offset,
					     int                  key_len,
					     void                *data,
					     void_f_pvoid_t      *free_f,
					     /*OUT*/s_xbt_dictelm_t **where);
static void         _dictelm_wrapper_free(void*);

static _XBT_INLINE void  _str_prefix_lgr(const char *key1,
					  int         key_len1,
					  const char *key2,
					  int         key_len2,
					  int        *offset,
					  int        *match);


static void _xbt_dictelm_dump_rec(s_xbt_dictelm_t *head,
				   int             offset,
				   void_f_pvoid_t *output);



static void _xbt_dictelm_set_rec(s_xbt_dictelm_t *head,
				  char           *key,
				  int             key_len,
				  int             offset,
				  void           *data,
				  void_f_pvoid_t *free_f);
static xbt_error_t _xbt_dictelm_get_rec(s_xbt_dictelm_t *head,
					       const char     *key,
					       int             key_len,
					       int             offset,
					       /* OUT */void **data);
static xbt_error_t _xbt_dictelm_remove_rec(s_xbt_dictelm_t *head,
					     const char     *key,
					     int             key_len,
					     int             offset);

static _XBT_INLINE
void
_collapse_if_need(s_xbt_dictelm_t *p_head,
		  int             pos,
		  int             offset);

/* ---- */

static _XBT_INLINE
void *
xbt_memdup(const void * const ptr,
	    const size_t       length) {
  void * new_ptr = NULL;

  new_ptr = xbt_malloc(length);
  memcpy(new_ptr, ptr, length);
   
  return new_ptr;
}

/*
 * _xbt_nibble_to_char:
 *
 * Change any byte to a printable char
 */

static _XBT_INLINE
char
_xbt_nibble_to_char(unsigned char c) {
  c &= 0x0f;
  return c>9 ? c-10+'a' : c + '0';
}

/*
 * _xbt_bytes_to_string:
 *
 * Change any byte array to a printable string
 * The length of string_container should at least be data_len*2+1 
 */
static _XBT_INLINE
char *
_xbt_bytes_to_string(char * const ptr,
                      int          data_len,
                      char * const string_container) {
  unsigned char *src = (unsigned char *)ptr;
           char *dst = string_container;

  while (data_len--) {
    *dst++ = _xbt_nibble_to_char(*src   & 0x0f     );
    *dst++ = _xbt_nibble_to_char(*src++ & 0xf0 >> 4);
  }

  *dst = 0;

  return ptr;
}

/* ---- */

/*
 * _xbt_dictelm_alloc:
 *
 * Alloc a dict element with no child.
 */
static _XBT_INLINE
void
_xbt_dictelm_alloc(char                *key,
		    int                  key_len,
		    int                  offset,
		    void                *data,
		    void_f_pvoid_t      *free_f,
                 /*OUT*/s_xbt_dictelm_t **pp_elm) {
  s_xbt_dictelm_t *p_elm  = NULL;

  p_elm = xbt_new(s_xbt_dictelm_t,1);

  p_elm->key      = key;
  p_elm->key_len  = key_len;
  p_elm->offset   = offset;
  p_elm->content  = data;
  p_elm->free_f = free_f;
  p_elm->sub      = xbt_dynar_new(sizeof(s_xbt_dictelm_t*), _dictelm_wrapper_free);

  *pp_elm = p_elm;

}

/**
 * xbt_dictelm_free:
 *
 * @pp_elm: the dict elem to be freed
 *
 * Frees a dictionnary element with all its childs.
 */
void
xbt_dictelm_free(s_xbt_dictelm_t **pp_elm)  {
  if (*pp_elm) {
    s_xbt_dictelm_t *p_elm = *pp_elm;

    xbt_dynar_free(&(p_elm->sub));

    if (p_elm->key) {
      free(p_elm->key);
    }

    if (p_elm->free_f && p_elm->content) {
      p_elm->free_f(p_elm->content);
    }

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
  DEBUG3("Free dictelm '%.*s' %p", 
	 (*(s_xbt_dictelm_t**)pp_elm)->key_len, (*(s_xbt_dictelm_t**)pp_elm)->key,
	 *(void**)pp_elm);
  xbt_dictelm_free((s_xbt_dictelm_t**)pp_elm);
}

/*####[ utility functions ]##################################################*/
/**
 * _str_prefix_lgr:
 *
 *
 * Returns the length of the common prefix of @str1 and @str2.
 * Do make sure the strings are not null
 */
static _XBT_INLINE
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

  /*CDEBUG5(dict_search, "%s: [%.*s] <=> [%.*s]", __FUNCTION__, 
            key1,key_len1,key2,key_len2);*/

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
 * convention than @_xbt_dict_child_search() )
 */
static _XBT_INLINE
void
_dict_child_cmp(s_xbt_dictelm_t *p_dict,
                int          pos,
                const char  *key,
                const int    key_len,
                int         *p_offset,
                int         *p_match,
                int         *p_cmp) {
  s_xbt_dictelm_t  *p_child = NULL;
  int           cmp     = 0;
  int           o       = *p_offset;
  int           m       = *p_match;

  p_child = xbt_dynar_get_as(p_dict->sub, pos, s_xbt_dictelm_t*);

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

  CDEBUG6(dict_search, "Cmp '%.*s' and '%.*s' (offset=%d) => %d", 
	  p_child->key_len - *p_offset, p_child->key + *p_offset,
	  key_len - *p_offset, key + *p_offset,
	  *p_offset,cmp);

 end:
  *p_offset = o;
  *p_match  = m;
  *p_cmp    = cmp;
}

/**
 * _xbt_dict_child_search:
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
static _XBT_INLINE
void
_xbt_dictelm_child_search(s_xbt_dictelm_t *p_elm,
			   const char  *key,
			   int          key_len,
			   int         *p_pos,
			   int         *p_offset,
			   int         *p_match) {

  int          p       = 0;
  int          o       = *p_offset;
  int          m       = 0;
  int          len     = 0;

  
  CDEBUG5(dict_search, "search child [%.*s] under [%.*s] (len=%lu)",
	  key_len, key,
          p_elm?p_elm->key_len:6, p_elm?p_elm->key:"(head)",
  	  (p_elm&&p_elm->sub)?xbt_dynar_length(p_elm->sub):0);
  

  len = xbt_dynar_length(p_elm->sub);

  if(1) {
    int p_min = 0;
    int p_max = len-1;
    int cmp = 0;

    p = p_min;
    if(len==0) {
      p=0;
    } else {
      _dict_child_cmp(p_elm, p_min, key, key_len, &o, &m, &cmp);
      if(!m) { /* OK, maybe it is somewhere else. */
	o = *p_offset;
	if (cmp<0) { /* Insert at the very beginning */
	  p=0;
	} else if (p_max<=0) { /* No way. It is not there. Insert à the very end */
	  p=p_max+1;
	  m = 0;
	} else { 
	  p=p_max;
	  _dict_child_cmp(p_elm, p_max, key, key_len, &o, &m, &cmp);
	  if(!m) {
	    o = *p_offset;
	    if(cmp>0) { /* Should be located at the end of the table */
	      p=p_max+1;
	    } else { /* Too bad, let's go for a dichotomic search. */
	      while(p_max-p_min>1) {
		_dict_child_cmp(p_elm, (p_min+p_max)/2, key, key_len, &o, &m, &cmp);
		if(m) break;
		o = *p_offset;
		if(cmp<0) p_max=(p_min+p_max)/2;
		if(cmp>0) p_min=(p_min+p_max)/2;
	      } 
	      if(m) /* We have the element */
		p=(p_min+p_max)/2 ;
	      else /* it should be inserted just after p_min */
		p=p_min + 1;
	    }
	  } 
	}
      }
    }
  } else {
    for (p = 0; p < len; p++) {
      int          cmp     = 0;
      
      _dict_child_cmp(p_elm, p, key, key_len, &o, &m, &cmp);
      
      if (m)
	break;
      
      o = *p_offset;
      m = 0;
    }
  }

  *p_offset = o;
  *p_pos    = p;
  *p_match  = m;
  CDEBUG5(dict_search, "search [%.*s] in [%.*s] => %s",
	  key_len, key,
          p_elm?p_elm->key_len:6, p_elm?p_elm->key:"(head)",
	  ( m == 0 ? "no child have a common prefix" :
	    ( m == 1 ? "selected child have exactly this key" :
	      ( m == 2 ? "selected child constitutes a prefix" :
		( m == 3 ? "key is a prefix of selected child" :
		  (m == 4 ? "selected child share a prefix" :
		   "internal error")))))
	  );  
}

/**
 * _xbt_dictelm_change_value:
 *
 * Change the value of the dictelm, making sure to free the old one, if any.
 */
static _XBT_INLINE
void
_xbt_dictelm_change_value(s_xbt_dictelm_t    *p_elm,
			   void           *data,
			   void_f_pvoid_t *free_f) {

  if (p_elm->content && p_elm->free_f) {
    p_elm->free_f(p_elm->content);
  }

  p_elm->free_f = free_f;
  p_elm->content  = data;
}

/**
 * _xbt_dictelm_set_rec:
 *
 * @head: the head of the dict
 * @key: the key to set the new data
 * @offset: offset on key.
 * @data: the data to add in the dict
 *
 * set the @data in the structure under the @key. The @key is destroyed
 * in the process. Think to strdup it before.
 *
 * This is a helper function to xbt_dict_set which locks the struct and
 * strdup the key before action. 
 */
void
_xbt_dictelm_set_rec(s_xbt_dictelm_t     *p_head,
			 char            *key,
			 int              key_len,
			 int              offset,
			 void            *data,
			 void_f_pvoid_t  *free_f) {
  int          match      = 0;
  int          pos        = 0;
  const int    old_offset = offset;

  CDEBUG6(dict_add, "--> Insert '%.*s' after '%.*s' (offset=%d) in tree %p",
	  key_len, key, 
	  ((p_head && p_head->key) ? p_head->key_len : 6),
	  ((p_head && p_head->key) ? p_head->key : "(head)"), 
	  offset, (void*)p_head);

  /*** The trivial cases first ***/

  /* there is no key (we did enough recursion), change the value of head */
  if (offset >= key_len) {

    CDEBUG0(dict_add, "--> Change the value of head");

    _xbt_dictelm_change_value(p_head, data, free_f);
    free(key); /* Keep the key used in the tree */

    return;
  }

  /*** Search where to add this child, and how ***/
  _xbt_dictelm_child_search(p_head, key, key_len, &pos, &offset, &match);

  CDEBUG3(dict_add, "child_search => pos=%d, offset=%d, match=%d",
	  pos, offset, match);

  switch (match) {

  case 0: /* no child have a common prefix */
    {
      s_xbt_dictelm_t *p_child = NULL;

      _xbt_dictelm_alloc(key, key_len, offset, data, free_f, &p_child);
      CDEBUG1(dict_add, "-> Add a child %p", (void*)p_child);
      xbt_dynar_insert_at(p_head->sub, pos, &p_child);

      return;
    }

  case 1: /* A child have exactly this key => change its value*/
    {
      s_xbt_dictelm_t *p_child = NULL;

      p_child = xbt_dynar_get_as(p_head->sub, pos, s_xbt_dictelm_t*);
      CDEBUG1(dict_add, "-> Change the value of the child %p", (void*)p_child);
      _xbt_dictelm_change_value(p_child, data, free_f);

      free(key);

      return;
    }

  case 2: /* A child constitutes a prefix of the key => recurse */
    {
      s_xbt_dictelm_t *p_child = NULL;

      p_child=xbt_dynar_get_as(p_head->sub, pos, s_xbt_dictelm_t*);
      CDEBUG2(dict_add,"-> Recurse on %p (offset=%d)", (void*)p_child, offset);

      _xbt_dictelm_set_rec(p_child, key, key_len, 
			    offset, data, free_f);
      return;
    }

  case 3: /* The key is a prefix of the child => child becomes child of p_new */
    {
      s_xbt_dictelm_t *p_new   = NULL;
      s_xbt_dictelm_t *p_child = NULL;

      p_child=xbt_dynar_get_as(p_head->sub, pos, s_xbt_dictelm_t*);
      _xbt_dictelm_alloc(key, key_len, old_offset, data, free_f, &p_new);

      CDEBUG2(dict_add, "-> The child %p become child of new dict (%p)",
              (void*)p_child, (void*)p_new);

      xbt_dynar_push(p_new->sub, &p_child);
      p_child->offset = offset;
      xbt_dynar_set(p_head->sub, pos, &p_new);

      return;
    }

  case 4: /* A child share a common prefix with this key => Common ancestor */
    {
      s_xbt_dictelm_t *p_new       = NULL;
      s_xbt_dictelm_t *p_child     = NULL;
      s_xbt_dictelm_t *p_anc       = NULL;
      char        *anc_key     = NULL;
      int          anc_key_len = offset;

      _xbt_dictelm_alloc(key, key_len, offset, data, free_f, &p_new);
      p_child=xbt_dynar_get_as(p_head->sub, pos, s_xbt_dictelm_t*);

      anc_key = xbt_memdup(key, anc_key_len);

      _xbt_dictelm_alloc(anc_key, anc_key_len, old_offset, NULL, NULL, &p_anc);

      CDEBUG3(dict_add, "-> Make a common ancestor %p (%.*s)",
	      (void*)p_anc, anc_key_len, anc_key);

      if (key[offset] < p_child->key[offset]) {
        xbt_dynar_push(p_anc->sub, &p_new);
        xbt_dynar_push(p_anc->sub, &p_child);
      } else {
        xbt_dynar_push(p_anc->sub, &p_child);
        xbt_dynar_push(p_anc->sub, &p_new);
      }

      p_child->offset = offset;

      xbt_dynar_set(p_head->sub, pos, &p_anc);

      return;
    }

  default:
    DIE_IMPOSSIBLE;
  }
}

/**
 * xbt_dictelm_set_ext:
 *
 * @head: the head of the dict
 * @key: the key to set the new data
 * @data: the data to add in the dict
 *
 * set the @data in the structure under the @key, which can be any kind 
 * of data, as long as its length is provided in @key_len.
 */
void
xbt_dictelm_set_ext(s_xbt_dictelm_t **pp_head,
			const char      *_key,
			int              key_len,
			void            *data,
			void_f_pvoid_t  *free_f) {
  s_xbt_dictelm_t  *p_head  = *pp_head;
  char         *key     =  NULL;

  key = xbt_memdup(_key, key_len);

  /* there is no head, create it */
  if (!p_head) {
    s_xbt_dictelm_t *p_child = NULL;

    CDEBUG0(dict_add, "Create an head");

    /* The head is priviledged by being the only one with a NULL key */
    _xbt_dictelm_alloc(NULL, 0, 0, NULL, NULL, &p_head);

    _xbt_dictelm_alloc(key, key_len, 0, data, free_f, &p_child);
    xbt_dynar_insert_at(p_head->sub, 0, &p_child);

    *pp_head = p_head;

    return;
  }

  _xbt_dictelm_set_rec(p_head, key, key_len, 0, data, free_f);
}

/**
 * xbt_dictelm_set:
 *
 * @head: the head of the dict
 * @key: the key to set the new data
 * @data: the data to add in the dict
 *
 * set the @data in the structure under the @key, which is a 
 * null terminated string.
 */
void
xbt_dictelm_set(s_xbt_dictelm_t **pp_head,
		    const char      *_key,
		    void            *data,
		    void_f_pvoid_t  *free_f) {

  xbt_dictelm_set_ext(pp_head, _key, 1+strlen(_key), data, free_f);
}

/**
 * _xbt_dict_get_rec:
 *
 * @head: the head of the dict
 * @key: the key to find data
 * @offset: offset on the key
 * @data: the data that we are looking for
 * @Returns: xbt_error
 *
 * Search the given @key. mismatch_error when not found.
 */
static 
xbt_error_t
_xbt_dictelm_get_rec(s_xbt_dictelm_t *p_head,
		      const char     *key,
		      int             key_len,
		      int             offset,
		      void **data) {

  CDEBUG3(dict_search, "Search %.*s in %p", key_len, key, (void*)p_head); 

  /*** The trivial case first ***/

  /* we did enough recursion, we're done */
  if (offset >= key_len) {
    *data = p_head->content;

    return no_error;
  }

  {
    int match = 0;
    int pos   = 0;

    *data = NULL; /* Make it ready to answer 'not found' in one operation */

    /*** Search where is the good child, and how good it is ***/
    _xbt_dictelm_child_search(p_head, key, key_len, &pos, &offset, &match);

    switch (match) {

    case 0: /* no child have a common prefix */
      return mismatch_error;

    case 1: /* A child have exactly this key => Got it */
      {
        s_xbt_dictelm_t *p_child = NULL;

        p_child = xbt_dynar_get_as(p_head->sub, pos, s_xbt_dictelm_t*);
        *data = p_child->content;

        return no_error;
      }

    case 2: /* A child constitutes a prefix of the key => recurse */
      {
        s_xbt_dictelm_t *p_child = NULL;

        p_child = xbt_dynar_get_as(p_head->sub, pos, s_xbt_dictelm_t*);

        return _xbt_dictelm_get_rec(p_child, key, key_len, offset, data);
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
 * xbt_dictelm_get_ext:
 *
 * @head: the head of the dict
 * @key: the key to find data
 * @data: the data that we are looking for
 * @Returns: xbt_error
 *
 * Search the given @key. mismatch_error when not found.
 */
xbt_error_t
xbt_dictelm_get_ext(s_xbt_dictelm_t *p_head,
			  const char     *key,
			  int             key_len,
			  /* OUT */void **data) {
  /* there is no head, go to hell */
  if (!p_head) {
    return mismatch_error;
  }

  return _xbt_dictelm_get_rec(p_head, key, key_len, 0, data);
}

/**
 * xbt_dictelm_get:
 *
 * @head: the head of the dict
 * @key: the key to find data
 * @data: the data that we are looking for
 * @Returns: xbt_error
 *
 * Search the given @key. mismatch_error when not found.
 */
xbt_error_t
xbt_dictelm_get(s_xbt_dictelm_t    *p_head,
                   const char     *key,
                   /* OUT */void **data) {

  return xbt_dictelm_get_ext(p_head, key, 1+strlen(key), data);
}

/*----[ _xbt_dict_collapse ]------------------------------------------------*/
static _XBT_INLINE
void
_collapse_if_need(xbt_dictelm_t head,
		  int            pos,
		  int            offset) {
  xbt_dictelm_t child = NULL;

  CDEBUG2(dict_collapse, "Collapse %d of %p... ", pos, (void*)head);

  if (pos >= 0) {
    /* Remove the child if |it's key| == 0 (meaning it's dead) */
    child = xbt_dynar_get_as(head->sub, pos, xbt_dictelm_t);

    if (offset >= child->key_len) {

      xbt_assert0(xbt_dynar_length(child->sub) == 0,
		   "Found a dead child with grand childs. Internal error");

      CDEBUG1(dict_collapse, "Remove dead child %p.... ", (void*)child);
      xbt_dynar_remove_at(head->sub, pos, &child);
      xbt_dictelm_free(&child);
    }
  }

  if (!head->key) {
    CDEBUG0(dict_collapse, "Do not collapse the head, you stupid programm");
    return;
  }

  if (head->content || head->free_f ||
      xbt_dynar_length(head->sub) != 1) {
    CDEBUG0(dict_collapse, "Cannot collapse");
    return; /* cannot collapse */
  }

  child = xbt_dynar_get_as(head->sub, 0, xbt_dictelm_t);

  /* Get the child's key as new key */
  CDEBUG2(dict_collapse,
	  "Do collapse with only child %.*s", child->key_len, child->key);

  head->content  = child->content;
  head->free_f = child->free_f;
  free(head->key);
  head->key      = child->key;
  head->key_len  = child->key_len;

  xbt_dynar_free_container(&(head->sub)) ;

  head->sub = child->sub;
  free(child);
}

/**
 * _xbt_dict_remove_rec:
 *
 * @head: the head of the dict
 * @key: the key of the data to be removed
 * @offset: offset on the key
 * @Returns: xbt_error_t
 *
 * Remove the entry associated with the given @key
 */
xbt_error_t
_xbt_dictelm_remove_rec(xbt_dictelm_t head,
			 const char  *key,
			 int          key_len,
			 int          offset) {
  xbt_error_t errcode = no_error;

  /* there is no key to search, we did enough recursion => kill current */
  if (offset >= key_len) {
    int killme = 0; /* do I need to suicide me ? */

    if (head->content && head->free_f) {
      head->free_f(head->content);
    }

    killme = !xbt_dynar_length(head->sub);
    head->content  = NULL;
    head->free_f = NULL;
    _collapse_if_need(head, -1, offset);

    if (killme) {
      DEBUG0("kill this node");
      head->key_len = 0; /* killme. Cleanup done one step higher in recursion */
    }

    return errcode;

  } else {
    int match      =      0;
    int pos        =      0;
    int old_offset = offset;

    /*** Search where is the good child, and how good it is ***/
    _xbt_dictelm_child_search(head, key, key_len, &pos, &offset, &match);

    switch (match) {

    case 1: /* A child have exactly this key           => recurse */
    case 2: /* A child constitutes a prefix of the key => recurse */

      {
        s_xbt_dictelm_t *p_child = NULL;

        p_child = xbt_dynar_get_as(head->sub, pos, s_xbt_dictelm_t*);
        /*DEBUG5("Recurse on child %d of %p to remove %.*s (prefix=%d)",
          pos, (void*)p_child, key+offset, key_len-offset,offset);*/
        TRY(_xbt_dictelm_remove_rec(p_child, key, key_len, offset));

        _collapse_if_need(head, pos, old_offset);
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
 * xbt_dictelm_remove_ext:
 *
 * @head: the head of the dict
 * @key: the key of the data to be removed
 * @Returns: xbt_error_t
 *
 * Remove the entry associated with the given @key
 */
xbt_error_t
xbt_dictelm_remove_ext(xbt_dictelm_t head,
			const char  *key,
			int          key_len) {
  /* there is no head, go to hell */
  if (!head) {
    RAISE0(mismatch_error, "there is no head, go to hell");
  }
  
  return _xbt_dictelm_remove_rec(head, key, key_len, 0);
}

/**
 * xbt_dictelm_remove:
 *
 * @head: the head of the dict
 * @key: the key of the data to be removed
 * @Returns: xbt_error_t
 *
 * Remove the entry associated with the given @key
 */
xbt_error_t
xbt_dictelm_remove(xbt_dictelm_t head,
		    const char     *key) {
  return _xbt_dictelm_remove_rec(head, key, 1+strlen(key),0);
}

/*----[ _xbt_dict_dump_rec ]------------------------------------------------*/
/* private function to do the job of xbt_dict_dump recursively              */
/*---------------------------------------------------------------------------*/
static
void
_xbt_dictelm_dump_rec(xbt_dictelm_t  head,
		       int             offset,
		       void_f_pvoid_t *output) {
  xbt_dictelm_t child   =     NULL;
  char          *key     =     NULL;
  int            key_len =        0;
  int            i       =        0;

  if (!head)
    return;

  printf("[%p] ", (void*)head);

  key     = head->key;
  key_len = head->key_len;

  if (key_len)
    printf ("  ");

  for (i = 0; i < offset; i++)
    printf("-");

  fflush(stdout);

  if (key) {

    if (!key_len) {
      printf ("HEAD");
    } else {
      char *key_string = NULL;

      key_string = xbt_malloc(key_len*2+1);
      _xbt_bytes_to_string(key, key_len, key_string);

      printf("%.*s|(%d)", key_len-offset, key_string + offset, offset);

      free(key_string);
    }

  }

  printf(" -> ");

  if (head->content) {

    if (output) {
      output(head->content);
    } else {
      printf("(data)");
    }

  } else {
    printf("(null)");
  }

  printf("    \t\t\t[ %lu child(s) ]\n", xbt_dynar_length(head->sub));

  xbt_dynar_foreach(head->sub, i, child) 
    _xbt_dictelm_dump_rec(child, child->offset, output);

}

/**
 * xbt_dictelm_dump:
 *
 * @head: the head of the dict
 * @output: a function to dump each data in the tree
 * @Returns: xbt_error_t
 *
 * Ouputs the content of the structure. (for debuging purpose). @ouput is a
 * function to output the data. If NULL, data won't be displayed.
 */

void
xbt_dictelm_dump(xbt_dictelm_t  head,
		  void_f_pvoid_t *output) {
  _xbt_dictelm_dump_rec(head, 0, output);
}

/**
 * xbt_dictelm_print_fct:
 *
 * @data:
 *
 * To dump multidict, this function dumps a dict
 */

void
xbt_dictelm_print_fct(void *data) {
  printf("tree %p", (void*)data);
}

