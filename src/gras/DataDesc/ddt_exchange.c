/* $Id$ */

/* ddt_exchange - send/recv data described                                  */

/* Copyright (c) 2003 Olivier Aumage.                                       */
/* Copyright (c) 2003, 2004, 2005 Martin Quinson.                           */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/DataDesc/datadesc_private.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_chunk_send/recv */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ddt_exchange,datadesc,
				 "Sending data over the network");
const char *gras_datadesc_cat_names[9] = { 
  "undefined", 
  "scalar", "struct", "union", "ref", "array", "ignored",
  "invalid"};

static gras_datadesc_type_t int_type = NULL;
static gras_datadesc_type_t pointer_type = NULL;    
static _XBT_INLINE void gras_dd_send_int(gras_socket_t sock,             int  i);
static _XBT_INLINE void gras_dd_recv_int(gras_socket_t sock, int r_arch, int *i);

static _XBT_INLINE xbt_error_t
gras_dd_alloc_ref(xbt_dict_t  refs,  long int     size,
		  char       **r_ref, long int     r_len,
		  char	     **l_ref, int detect_cycle);

static _XBT_INLINE int
gras_dd_is_r_null(char **r_ptr, long int length);

static _XBT_INLINE void
gras_dd_send_int(gras_socket_t sock,int i) {

  if (!int_type) {
    int_type = gras_datadesc_by_name("int");
     xbt_assert(int_type);  
  }
   
  DEBUG1("send_int(%d)",i);
  gras_trp_chunk_send(sock, (char*)&i, int_type->size[GRAS_THISARCH]);
}

static _XBT_INLINE void
gras_dd_recv_int(gras_socket_t sock, int r_arch, int *i) {

  if (!int_type) {
     int_type = gras_datadesc_by_name("int");
     xbt_assert(int_type);
  }

  if (int_type->size[GRAS_THISARCH] >= int_type->size[r_arch]) {
    gras_trp_chunk_recv(sock, (char*)i, int_type->size[r_arch]);
    if (r_arch != GRAS_THISARCH)
      gras_dd_convert_elm(int_type,1,r_arch, i,i);
  } else {
    void *ptr = xbt_malloc(int_type->size[r_arch]);

    gras_trp_chunk_recv(sock, (char*)ptr, int_type->size[r_arch]);
    if (r_arch != GRAS_THISARCH)
      gras_dd_convert_elm(int_type,1,r_arch, ptr,i);
    free(ptr);
  }
  DEBUG1("recv_int(%d)",*i);
}

/*
 * Note: here we suppose that the remote NULL is a sequence 
 *       of 'length' bytes set to 0.
 * FIXME: Check in configure?
 */
static _XBT_INLINE int 
gras_dd_is_r_null(char **r_ptr, long int length) {
  int i;

  for (i=0; i<length; i++) {
    if ( ((unsigned char*)r_ptr) [i]) {
      return 0;
    }
  }

  return 1;
}

static _XBT_INLINE xbt_error_t
gras_dd_alloc_ref(xbt_dict_t  refs,
		  long int     size,
		  char       **r_ref,
		  long int     r_len, /* pointer_type->size[r_arch] */
		  char	     **l_ref,
		  int          detect_cycle) {
  char *l_data = NULL;

  xbt_assert1(size>0,"Cannot allocate %ld bytes!", size);
  l_data = xbt_malloc((size_t)size);

  *l_ref = l_data;
  DEBUG5("alloc_ref: l_data=%p, &l_data=%p; r_ref=%p; *r_ref=%p, r_len=%ld",
	 (void*)l_data,(void*)&l_data,
	 (void*)r_ref, (void*)(r_ref?*r_ref:NULL), r_len);
  if (detect_cycle && r_ref && !gras_dd_is_r_null( r_ref, r_len)) {
    void *ptr = xbt_malloc(sizeof(void *));

    CRITICAL0("Check for cycles");
    memcpy(ptr,l_ref, sizeof(void *));

    DEBUG2("Insert l_ref=%p under r_ref=%p",*(void**)ptr, *(void**)r_ref);

    if (detect_cycle)
       xbt_dict_set_ext(refs,(const char *) r_ref, r_len, ptr, free);
  }
  return no_error;
}

/**
 * gras_datadesc_cpy:
 *
 * Copy the data pointed by src and described by type 
 * to a new location, and store a pointer to it in dst.
 *
 */
xbt_error_t gras_datadesc_cpy(gras_datadesc_type_t type, 
			       void *src, 
			       void **dst) {
  THROW_UNIMPLEMENTED;
}

/***
 *** Direct use functions
 ***/

static void
gras_datadesc_send_rec(gras_socket_t         sock,
		       gras_cbps_t           state,
		       xbt_dict_t           refs,
		       gras_datadesc_type_t  type, 
		       char                 *data,
		       int                   detect_cycle) {

  xbt_ex_t             e;
  int                  cpt;
  gras_datadesc_type_t sub_type; /* type on which we recurse */
  
  VERB2("Send a %s (%s)", 
	type->name, gras_datadesc_cat_names[type->category_code]);

  if (type->send) {
    type->send(type,state,data);
  }

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    gras_trp_chunk_send(sock, data, type->size[GRAS_THISARCH]);
    break;

  case e_gras_datadesc_type_cat_struct: {
    gras_dd_cat_struct_t struct_data;
    gras_dd_cat_field_t  field;
    char                *field_data;
    
    struct_data = type->category.struct_data;
    xbt_assert1(struct_data.closed,
      "Please call gras_datadesc_declare_struct_close on %s before sending it",
		type->name);
    VERB1(">> Send all fields of the structure %s",type->name);
    xbt_dynar_foreach(struct_data.fields, cpt, field) {
      field_data = data;
      field_data += field->offset[GRAS_THISARCH];
      
      sub_type = field->type;
      
      if (field->send)
	field->send(type,state,field_data);
      
      VERB1("Send field %s",field->name);
      gras_datadesc_send_rec(sock,state,refs,sub_type, field_data, 
			     detect_cycle || sub_type->cycle);
      
    }
    VERB1("<< Sent all fields of the structure %s", type->name);
    
    break;
  }

  case e_gras_datadesc_type_cat_union: {
    gras_dd_cat_union_t union_data;
    gras_dd_cat_field_t field=NULL;
    int                 field_num;
    
    union_data = type->category.union_data;
    
    xbt_assert1(union_data.closed,
		"Please call gras_datadesc_declare_union_close on %s before sending it",
		type->name);
    /* retrieve the field number */
    field_num = union_data.selector(type, state, data);
    
    xbt_assert1(field_num > 0,
		 "union field selector of %s gave a negative value", 
		 type->name);
    
    xbt_assert3(field_num < xbt_dynar_length(union_data.fields),
	 "union field selector of %s returned %d but there is only %lu fields",
		 type->name, field_num, xbt_dynar_length(union_data.fields));

    /* Send the field number */
    gras_dd_send_int(sock, field_num);
    
    /* Send the content */
    field = xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
    sub_type = field->type;
    
    if (field->send)
      field->send(type,state,data);
    
    gras_datadesc_send_rec(sock,state,refs, sub_type, data, 
			   detect_cycle || sub_type->cycle);
          
    break;
  }
    
  case e_gras_datadesc_type_cat_ref: {
    gras_dd_cat_ref_t      ref_data;
    void                 **ref=(void**)data;
    int                    reference_is_to_send;
    
    ref_data = type->category.ref_data;
    
    /* Detect the referenced type and send it to peer if needed */
    sub_type = ref_data.type;
    if (sub_type == NULL) {
      sub_type = (*ref_data.selector)(type,state,data);
      gras_dd_send_int(sock, sub_type->code);
    }
    
    /* Send the actual value of the pointer for cycle handling */
    if (!pointer_type) {
      pointer_type = gras_datadesc_by_name("data pointer");
      xbt_assert(pointer_type);
    }
     
    gras_trp_chunk_send(sock, (char*)data,
			pointer_type->size[GRAS_THISARCH]);
    
    /* Send the pointed data only if not already sent */
    if (*(void**)data == NULL) {
      VERB0("Not sending NULL referenced data");
      break;
    }

    reference_is_to_send = 0;
    TRY {
      if (detect_cycle)
	/* return ignored. Just checking whether it's known or not */
	xbt_dict_get_ext(refs,(char*)ref, sizeof(char*));
      else 
	reference_is_to_send = 1;
    } CATCH(e) {
      if (e.category != not_found_error)
	RETHROW;
      reference_is_to_send = 1;
      xbt_ex_free(e);
    }

    if (reference_is_to_send) {
       VERB1("Sending data referenced at %p", (void*)*ref);
       if (detect_cycle)
	 xbt_dict_set_ext(refs, (char*)ref, sizeof(void*), ref, NULL);
       gras_datadesc_send_rec(sock,state,refs, sub_type, *ref, 
			      detect_cycle || sub_type->cycle);
	  
    } else {
       VERB1("Not sending data referenced at %p (already done)", (void*)*ref);
    } 
    
    break;
  }

  case e_gras_datadesc_type_cat_array: {
    gras_dd_cat_array_t    array_data;
    long int               count;
    char                  *ptr=data;
    long int               elm_size;
    
    array_data = type->category.array_data;
    
    /* determine and send the element count */
    count = array_data.fixed_size;
    if (count == 0) {
      count = array_data.dynamic_size(type,state,data);
      xbt_assert1(count >=0,
		   "Invalid (negative) array size for type %s",type->name);
      gras_dd_send_int(sock, count);
    }
    
    /* send the content */
    sub_type = array_data.type;
    elm_size = sub_type->aligned_size[GRAS_THISARCH];
    if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
      VERB1("Array of %ld scalars, send it in one shot",count);
      gras_trp_chunk_send(sock, data, 
			  sub_type->aligned_size[GRAS_THISARCH] * count);
    } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
	       sub_type->category.array_data.fixed_size > 0 &&
	       sub_type->category.array_data.type->category_code == e_gras_datadesc_type_cat_scalar) {
       
      VERB1("Array of %ld fixed array of scalars, send it in one shot",count);
      gras_trp_chunk_send(sock, data, 
			  sub_type->category.array_data.type->aligned_size[GRAS_THISARCH] 
			  * count * sub_type->category.array_data.fixed_size);
       
    } else {
      for (cpt=0; cpt<count; cpt++) {
	gras_datadesc_send_rec(sock,state,refs, sub_type, ptr, 
			       detect_cycle || sub_type->cycle);
	ptr += elm_size;
      }
    }
    break;
  }

  default:
    xbt_assert0(0, "Invalid type");
  }
}

/**
 * gras_datadesc_send:
 *
 * Copy the data pointed by src and described by type to the socket
 *
 */
void gras_datadesc_send(gras_socket_t        sock, 
			gras_datadesc_type_t type, 
			void *src) {

  xbt_ex_t e;
  gras_cbps_t  state;
  xbt_dict_t  refs; /* all references already sent */
 
  xbt_assert0(type,"called with NULL type descriptor");

  refs = xbt_dict_new();
  state = gras_cbps_new();
  
  TRY {
    gras_datadesc_send_rec(sock,state,refs,type,(char*)src, type->cycle);
  } CLEANUP {
    xbt_dict_free(&refs);
    gras_cbps_free(&state);
  } CATCH(e) {
    RETHROW;
  }
}

/**
 * gras_datadesc_recv_rec:
 *
 * Do the data reception job recursively.
 *
 * subsize used only to deal with vicious case of reference to dynamic array.
 *  This size is needed at the reference reception level (to allocate enough 
 * space) and at the array reception level (to fill enough room). 
 * 
 * Having this size passed as an argument of the recursive function is a crude
 * hack, but I was told that working code is sometimes better than neat one ;)
 */
static void
gras_datadesc_recv_rec(gras_socket_t         sock, 
		       gras_cbps_t           state,
		       xbt_dict_t           refs,
		       gras_datadesc_type_t  type,
		       int                   r_arch,
		       char                **r_data,
		       long int              r_lgr,
		       char                 *l_data,
		       int                   subsize,
		       int                   detect_cycle) {

  int                  cpt;
  gras_datadesc_type_t sub_type;
  xbt_ex_t e;

  VERB2("Recv a %s @%p", type->name, (void*)l_data);
  xbt_assert(l_data);

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    if (type->size[GRAS_THISARCH] == type->size[r_arch]) {
      gras_trp_chunk_recv(sock, (char*)l_data, type->size[r_arch]);
      if (r_arch != GRAS_THISARCH)
	gras_dd_convert_elm(type,1,r_arch, l_data,l_data);
    } else {
      void *ptr = xbt_malloc(type->size[r_arch]);

      gras_trp_chunk_recv(sock, (char*)ptr, type->size[r_arch]);
      if (r_arch != GRAS_THISARCH)
	gras_dd_convert_elm(type,1,r_arch, ptr,l_data);
      free(ptr);
    }
    break;

  case e_gras_datadesc_type_cat_struct: {
    gras_dd_cat_struct_t struct_data;
    gras_dd_cat_field_t  field;

    struct_data = type->category.struct_data;

    xbt_assert1(struct_data.closed,
		"Please call gras_datadesc_declare_struct_close on %s before receiving it",
		type->name);
    VERB1(">> Receive all fields of the structure %s",type->name);
    xbt_dynar_foreach(struct_data.fields, cpt, field) {
      char                 *field_data = l_data + field->offset[GRAS_THISARCH];

      sub_type = field->type;

      gras_datadesc_recv_rec(sock,state,refs, sub_type,
			     r_arch,NULL,0,
			     field_data,-1, 
			     detect_cycle || sub_type->cycle);
       
      if (field->recv)
        field->recv(type,state,(void*)l_data);
    
    }
    VERB1("<< Received all fields of the structure %s", type->name);
    
    break;
  }

  case e_gras_datadesc_type_cat_union: {
    gras_dd_cat_union_t union_data;
    gras_dd_cat_field_t field=NULL;
    int                 field_num;

    union_data = type->category.union_data;

    xbt_assert1(union_data.closed,
		"Please call gras_datadesc_declare_union_close on %s before receiving it",
		type->name);
    /* retrieve the field number */
    gras_dd_recv_int(sock, r_arch, &field_num);
    if (field_num < 0)
      THROW1(mismatch_error,0,
	     "Received union field for %s is negative", type->name);
    if (field_num < xbt_dynar_length(union_data.fields)) 
      THROW3(mismatch_error,0,
	     "Received union field for %s is said to be #%d but there is only %lu fields",
	     type->name, field_num, xbt_dynar_length(union_data.fields));
    
    /* Recv the content */
    field = xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
    sub_type = field->type;
    
    gras_datadesc_recv_rec(sock,state,refs, sub_type,
			   r_arch,NULL,0,
			   l_data,-1,
			   detect_cycle || sub_type->cycle);
    if (field->recv)
       field->recv(type,state,l_data);

    break;
  }

  case e_gras_datadesc_type_cat_ref: {
    char             **r_ref = NULL;
    char             **l_ref = NULL;
    gras_dd_cat_ref_t  ref_data;
    int reference_is_to_recv = 0;
    
    ref_data = type->category.ref_data;

    /* Get the referenced type locally or from peer */
    sub_type = ref_data.type;
    if (sub_type == NULL) {
      int ref_code;
      gras_dd_recv_int(sock, r_arch, &ref_code);
      sub_type = gras_datadesc_by_id(ref_code);
    }

    /* Get the actual value of the pointer for cycle handling */
    if (!pointer_type) {
      pointer_type = gras_datadesc_by_name("data pointer");
      xbt_assert(pointer_type);
    }

    r_ref = xbt_malloc(pointer_type->size[r_arch]);

    gras_trp_chunk_recv(sock, (char*)r_ref,
			pointer_type->size[r_arch]);

    /* Receive the pointed data only if not already sent */
    if (gras_dd_is_r_null(r_ref, pointer_type->size[r_arch])) {
      VERB1("Not receiving data remotely referenced @%p since it's NULL",
	    *(void **)r_ref);
      *(void**)l_data = NULL;
      free(r_ref);
      break;
    }
         
    reference_is_to_recv = 0;
    TRY {
      if (detect_cycle)
	l_ref = xbt_dict_get_ext(refs, (char*)r_ref, pointer_type->size[r_arch]);
      else 
	reference_is_to_recv = 1;
    } CATCH(e) {
      if (e.category != not_found_error)
        RETHROW;
      reference_is_to_recv = 1;
      xbt_ex_free(e);
    }
    if (reference_is_to_recv) {
      int subsubcount = 0;
      void *l_referenced=NULL;

      VERB2("Receiving a ref to '%s', remotely @%p",
	    sub_type->name, *(void**)r_ref);
      if (sub_type->category_code == e_gras_datadesc_type_cat_array) {
	/* Damn. Reference to a dynamic array. Allocating the space for it 
	   is more complicated */
	gras_dd_cat_array_t array_data = sub_type->category.array_data;
	gras_datadesc_type_t subsub_type;

	subsubcount = array_data.fixed_size;
	if (subsubcount == 0)
	  gras_dd_recv_int(sock, r_arch, &subsubcount);

	subsub_type = array_data.type;


	gras_dd_alloc_ref(refs,
			  subsub_type->size[GRAS_THISARCH] * subsubcount, 
			  r_ref,pointer_type->size[r_arch], 
			  (char**)&l_referenced,
			  detect_cycle);
      } else {
	gras_dd_alloc_ref(refs,sub_type->size[GRAS_THISARCH], 
			  r_ref,pointer_type->size[r_arch], 
			  (char**)&l_referenced,
			  detect_cycle);
      }

      gras_datadesc_recv_rec(sock,state,refs, sub_type,
			     r_arch,r_ref,pointer_type->size[r_arch],
			     (char*)l_referenced, subsubcount,
			     detect_cycle || sub_type->cycle);
			       
      *(void**)l_data=l_referenced;
      VERB3("'%s' remotely referenced at %p locally at %p",
	    sub_type->name, *(void**)r_ref, l_referenced);
      
    } else {
      VERB2("NOT receiving data remotely referenced @%p (already done, @%p here)",
	    *(void**)r_ref, *(void**)l_ref);

      *(void**)l_data=*l_ref;

    } 
    free(r_ref);
    break;
  }

  case e_gras_datadesc_type_cat_array: {
    gras_dd_cat_array_t    array_data;
    int       count;
    char     *ptr;
    long int  elm_size;

    array_data = type->category.array_data;
    /* determine element count locally, or from caller, or from peer */
    count = array_data.fixed_size;
    if (count == 0)
      count = subsize;
    if (count == 0)
      gras_dd_recv_int(sock, r_arch, &count);
    if (count == 0)
      THROW1(mismatch_error,0,
	     "Invalid (=0) array size for type %s",type->name);

    /* receive the content */
    sub_type = array_data.type;
    if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
      VERB1("Array of %d scalars, get it in one shoot", count);
      if (sub_type->aligned_size[GRAS_THISARCH] >= 
	  sub_type->aligned_size[r_arch]) {
	gras_trp_chunk_recv(sock, (char*)l_data, 
			    sub_type->aligned_size[r_arch] * count);
	if (r_arch != GRAS_THISARCH)
	  gras_dd_convert_elm(sub_type,count,r_arch, l_data,l_data);
      } else {
	ptr = xbt_malloc(sub_type->aligned_size[r_arch] * count);

	gras_trp_chunk_recv(sock, (char*)ptr, 
			    sub_type->size[r_arch] * count);
	if (r_arch != GRAS_THISARCH)
	  gras_dd_convert_elm(sub_type,count,r_arch, ptr,l_data);
	free(ptr);
      }
    } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
	       sub_type->category.array_data.fixed_size > 0 &&
	       sub_type->category.array_data.type->category_code == e_gras_datadesc_type_cat_scalar) {
      gras_datadesc_type_t subsub_type;
      array_data = sub_type->category.array_data;
      subsub_type = array_data.type;
       
      VERB1("Array of %d fixed array of scalars, get it in one shot",count);
      if (subsub_type->aligned_size[GRAS_THISARCH] >= 
	  subsub_type->aligned_size[r_arch]) {
	gras_trp_chunk_recv(sock, (char*)l_data, 
			    subsub_type->aligned_size[r_arch] * count * 
			    array_data.fixed_size);
	if (r_arch != GRAS_THISARCH)
	  gras_dd_convert_elm(subsub_type,count*array_data.fixed_size,r_arch, l_data,l_data);
      } else {
	ptr = xbt_malloc(subsub_type->aligned_size[r_arch] * count*array_data.fixed_size);

	gras_trp_chunk_recv(sock, (char*)ptr, 
			    subsub_type->size[r_arch] * count*array_data.fixed_size);
	if (r_arch != GRAS_THISARCH)
	  gras_dd_convert_elm(subsub_type,count*array_data.fixed_size,r_arch, ptr,l_data);
	free(ptr);
      }
      
       
    } else {
      /* not scalar content, get it recursively (may contain pointers) */
      elm_size = sub_type->aligned_size[GRAS_THISARCH];
      VERB2("Receive a %d-long array of %s",count, sub_type->name);

      ptr = l_data;
      for (cpt=0; cpt<count; cpt++) {
	gras_datadesc_recv_rec(sock,state,refs, sub_type,
			       r_arch, NULL, 0, ptr,-1,
			       detect_cycle || sub_type->cycle);
				   
	ptr += elm_size;
      }
    }
    break;
  }
        
  default:
    xbt_assert0(0, "Invalid type");
  }
  
  if (type->recv)
    type->recv(type,state,l_data);

}

/**
 * gras_datadesc_recv:
 *
 * Get an instance of the datatype described by @type from the @socket, 
 * and store a pointer to it in @dst
 *
 */
void
gras_datadesc_recv(gras_socket_t         sock, 
		   gras_datadesc_type_t  type,
		   int                   r_arch,
		   void                 *dst) {

  xbt_ex_t e;
  gras_cbps_t  state; /* callback persistent state */
  xbt_dict_t  refs;  /* all references already sent */

  refs = xbt_dict_new();
  state = gras_cbps_new();

  xbt_assert0(type,"called with NULL type descriptor");
  TRY {
    gras_datadesc_recv_rec(sock, state, refs, type, 
			   r_arch, NULL, 0,
			   (char *) dst,-1, 
			   type->cycle);
  } CLEANUP {
    xbt_dict_free(&refs);
    gras_cbps_free(&state);
  } CATCH(e) {
    RETHROW;
  }
}

#if 0
/***
 *** IDL compiling functions
 ***/

#define gras_datadesc_send_rec foo /* Just to make sure the copypast was ok */
#define gras_datadesc_send     foo /* Just to make sure the copypast was ok */
#define gras_datadesc_recv_rec foo /* Just to make sure the copypast was ok */
#define gras_datadesc_recv     foo /* Just to make sure the copypast was ok */

static xbt_error_t 
gras_datadesc_gen_send_rec(gras_socket_t         sock,
			   gras_cbps_t           state,
			   xbt_dict_t           refs,
			   gras_datadesc_type_t  type, 
			   char                 *data,
			   int                   detect_cycle) {

  xbt_error_t         errcode;
  int                  cpt;
  gras_datadesc_type_t sub_type; /* type on which we recurse */
  
  printf("  VERB2(\"Send a %s (%s)\");\n", 
	 type->name, gras_datadesc_cat_names[type->category_code]);

  xbt_assert0(!type->send, "Callbacks not implemented in IDL compiler");

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    printf("  TRYOLD(gras_trp_chunk_send(sock, data, %lu));\n",type->size[GRAS_THISARCH]);
    break;

  case e_gras_datadesc_type_cat_struct: {
    gras_dd_cat_struct_t struct_data;
    gras_dd_cat_field_t  field;
    char                *field_data;
    
    struct_data = type->category.struct_data;
    xbt_assert1(struct_data.closed,
      "Please call gras_datadesc_declare_struct_close on %s before sending it",
		type->name);
    printf("  VERB1(\">> Send all fields of the structure %s\");\n",type->name);
    xbt_dynar_foreach(struct_data.fields, cpt, field) {
      field_data = data;
      field_data += field->offset[GRAS_THISARCH];
      
      sub_type = field->type;
      
      xbt_assert0(!field->send, "Callbacks not implemented in IDL compiler");
      
      printf("  VERB1(\"Send field %s\");\n",field->name);
      printf("  data += %lu;\n",field->offset[GRAS_THISARCH]);
      TRYOLD(gras_datadesc_gen_send_rec(sock,state,refs,sub_type, field_data, 
				     detect_cycle || sub_type->cycle));
      printf("  data -= %lu;\n",field->offset[GRAS_THISARCH]);
      
      xbt_assert0(!field->recv, "Callbacks not implemented in IDL compiler");
    }
    printf("  VERB1(\"<< Sent all fields of the structure %s\"", type->name);
    
    break;
  }

  case e_gras_datadesc_type_cat_union: {
    gras_dd_cat_union_t union_data;
    gras_dd_cat_field_t field=NULL;
    int                 field_num;
    
    union_data = type->category.union_data;
    
    xbt_assert1(union_data.closed,
		"Please call gras_datadesc_declare_union_close on %s before sending it",
		type->name);
    /* retrieve the field number */
    printf("  field_num = union_data.selector(state, data);\n");
    
    printf("  xbt_assert0(field_num > 0,\n");
    printf("              \"union field selector of %s gave a negative value\");\n",type->name);
    
    printf("  xbt_assert3(field_num < xbt_dynar_length(union_data.fields),\n");
    printf("              \"union field selector of %s returned %%d but there is only %lu fields\",field_num);\n",
		 type->name, xbt_dynar_length(union_data.fields));

    /* Send the field number */
    printf("TRYOLD(gras_dd_send_int(sock, field_num));\n");
    
    /* Send the content */
    field = xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
    sub_type = field->type;
    
    if (field->send)
      field->send(state,data);
    
    TRYOLD(gras_datadesc_gen_send_rec(sock,state,refs, sub_type, data,
				   detect_cycle || sub_type->cycle));
           
    break;
  }
    
  case e_gras_datadesc_type_cat_ref: {
    gras_dd_cat_ref_t      ref_data;

    void                 **ref=(void**)data;
    void *dummy;
    
    ref_data = type->category.ref_data;
    
    /* Detect the referenced type and send it to peer if needed */
    sub_type = ref_data.type;
    if (sub_type == NULL) {
      sub_type = (*ref_data.selector)(state,data);
      TRYOLD(gras_dd_send_int(sock, sub_type->code));
    }
    
    /* Send the actual value of the pointer for cycle handling */
    if (!pointer_type) {
      pointer_type = gras_datadesc_by_name("data pointer");
      xbt_assert(pointer_type);
    }
     
    TRYOLD(gras_trp_chunk_send(sock, (char*)data,
			    pointer_type->size[GRAS_THISARCH]));
    
    /* Send the pointed data only if not already sent */
    if (*(void**)data == NULL) {
      VERB0("Not sending NULL referenced data");
      break;
    }
    errcode = detect_cycle 
            ? xbt_dict_get_ext(refs,(char*)ref, sizeof(void*), &dummy)
            : mismatch_error;
    if (errcode == mismatch_error) {
       VERB1("Sending data referenced at %p", (void*)*ref);
       if (detect_cycle)
	 xbt_dict_set_ext(refs, (char*)ref, sizeof(void*), ref, NULL);
       TRYOLD(gras_datadesc_gen_send_rec(sock,state,refs, sub_type, *ref, 
				      detect_cycle || sub_type->cycle));
	  
    } else if (errcode == no_error) {
       VERB1("Not sending data referenced at %p (already done)", (void*)*ref);
    } else {
       return errcode;
    }
    
    break;
  }

  case e_gras_datadesc_type_cat_array: {
    gras_dd_cat_array_t    array_data;
    long int               count;
    char                  *ptr=data;
    long int               elm_size;
    
    array_data = type->category.array_data;
    
    /* determine and send the element count */
    count = array_data.fixed_size;
    if (count == 0) {
      count = array_data.dynamic_size(state,data);
      xbt_assert1(count >=0,
		   "Invalid (negative) array size for type %s",type->name);
      TRYOLD(gras_dd_send_int(sock, count));
    }
    
    /* send the content */
    sub_type = array_data.type;
    elm_size = sub_type->aligned_size[GRAS_THISARCH];
    if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
      VERB1("Array of %ld scalars, send it in one shot",count);
      TRYOLD(gras_trp_chunk_send(sock, data, 
			      sub_type->aligned_size[GRAS_THISARCH] * count));
    } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
	       sub_type->category.array_data.fixed_size > 0 &&
	       sub_type->category.array_data.type->category_code == e_gras_datadesc_type_cat_scalar) {
       
      VERB1("Array of %ld fixed array of scalars, send it in one shot",count);
      TRYOLD(gras_trp_chunk_send(sock, data, 
			      sub_type->category.array_data.type->aligned_size[GRAS_THISARCH] 
			         * count * sub_type->category.array_data.fixed_size));
       
    } else {
      for (cpt=0; cpt<count; cpt++) {
	TRYOLD(gras_datadesc_gen_send_rec(sock,state,refs, sub_type, ptr, 
				       detect_cycle || sub_type->cycle));
	ptr += elm_size;
      }
    }
    break;
  }

  default:
    xbt_assert0(0, "Invalid type");
  }

  return no_error;
}

/**
 * gras_datadesc_gen_send:
 *
 * Copy the data pointed by src and described by type to the socket
 *
 */
xbt_error_t gras_datadesc_gen_send(gras_socket_t        sock, 
				   gras_datadesc_type_t type, 
				   void *src) {

  xbt_error_t errcode;
  gras_cbps_t  state;
  xbt_dict_t  refs; /* all references already sent */
 
  refs = xbt_dict_new();
  state = gras_cbps_new();
   
  printf("xbt_error_t gras_%s_send(gras_socket_t sock,void *dst){\n",
	 type->name);
  errcode = gras_datadesc_gen_send_rec(sock,state,refs,type,(char*)src, 
				       detect_cycle || sub_type->cycle);
  printf("}\n");
  
  xbt_dict_free(&refs);
  gras_cbps_free(&state);

  return errcode;
}

/**
 * gras_datadesc_gen_recv_rec:
 *
 * Do the data reception job recursively.
 *
 * subsize used only to deal with vicious case of reference to dynamic array.
 *  This size is needed at the reference reception level (to allocate enough 
 * space) and at the array reception level (to fill enough room). 
 * 
 * Having this size passed as an argument of the recursive function is a crude
 * hack, but I was told that working code is sometimes better than neat one ;)
 */
static xbt_error_t
gras_datadesc_gen_recv_rec(gras_socket_t         sock, 
			   gras_cbps_t           state,
			   xbt_dict_t           refs,
			   gras_datadesc_type_t  type,
			   int                   r_arch,
			   char                **r_data,
			   long int              r_lgr,
			   char                 *l_data,
			   int                   subsize,
			   int                   detect_cycle) {

  xbt_error_t         errcode;
  int                  cpt;
  gras_datadesc_type_t sub_type;

  VERB2("Recv a %s @%p", type->name, (void*)l_data);
  xbt_assert(l_data);

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    if (type->size[GRAS_THISARCH] == type->size[r_arch]) {
      TRYOLD(gras_trp_chunk_recv(sock, (char*)l_data, type->size[r_arch]));
      if (r_arch != GRAS_THISARCH)
	TRYOLD(gras_dd_convert_elm(type,1,r_arch, l_data,l_data));
    } else {
      void *ptr = xbt_malloc(type->size[r_arch]);

      TRYOLD(gras_trp_chunk_recv(sock, (char*)ptr, type->size[r_arch]));
      if (r_arch != GRAS_THISARCH)
	TRYOLD(gras_dd_convert_elm(type,1,r_arch, ptr,l_data));
      free(ptr);
    }
    break;

  case e_gras_datadesc_type_cat_struct: {
    gras_dd_cat_struct_t struct_data;
    gras_dd_cat_field_t  field;

    struct_data = type->category.struct_data;

    xbt_assert1(struct_data.closed,
		"Please call gras_datadesc_declare_struct_close on %s before receiving it",
		type->name);
    VERB1(">> Receive all fields of the structure %s",type->name);
    xbt_dynar_foreach(struct_data.fields, cpt, field) {
      char                 *field_data = l_data + field->offset[GRAS_THISARCH];

      sub_type = field->type;

      TRYOLD(gras_datadesc_gen_recv_rec(sock,state,refs, sub_type,
				     r_arch,NULL,0,
				     field_data,-1,
				     detect_cycle || sub_type->cycle));
      if (field->recv)
        field->recv(type,state,data);

    }
    VERB1("<< Received all fields of the structure %s", type->name);
    
    break;
  }

  case e_gras_datadesc_type_cat_union: {
    gras_dd_cat_union_t union_data;
    gras_dd_cat_field_t field=NULL;
    int                 field_num;

    union_data = type->category.union_data;

    xbt_assert1(union_data.closed,
		"Please call gras_datadesc_declare_union_close on %s before receiving it",
		type->name);
    /* retrieve the field number */
    TRYOLD(gras_dd_recv_int(sock, r_arch, &field_num));
    if (field_num < 0)
      RAISE1(mismatch_error,
	     "Received union field for %s is negative", type->name);
    if (field_num < xbt_dynar_length(union_data.fields)) 
      RAISE3(mismatch_error,
	     "Received union field for %s is %d but there is only %lu fields",
	     type->name, field_num, xbt_dynar_length(union_data.fields));
    
    /* Recv the content */
    field = xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
    sub_type = field->type;
    
    TRYOLD(gras_datadesc_gen_recv_rec(sock,state,refs, sub_type,
				   r_arch,NULL,0,
				   l_data,-1,
				   detect_cycle || sub_type->cycle));
    if (field->recv)
        field->recv(type,state,data);
		  
    break;
  }

  case e_gras_datadesc_type_cat_ref: {
    char             **r_ref = NULL;
    char             **l_ref = NULL;
    gras_dd_cat_ref_t  ref_data;
    
    ref_data = type->category.ref_data;

    /* Get the referenced type locally or from peer */
    sub_type = ref_data.type;
    if (sub_type == NULL) {
      int ref_code;
      TRYOLD(gras_dd_recv_int(sock, r_arch, &ref_code));
      TRYOLD(gras_datadesc_by_id(ref_code, &sub_type));
    }

    /* Get the actual value of the pointer for cycle handling */
    if (!pointer_type) {
      pointer_type = gras_datadesc_by_name("data pointer");
      xbt_assert(pointer_type);
    }

    r_ref = xbt_malloc(pointer_type->size[r_arch]);

    TRYOLD(gras_trp_chunk_recv(sock, (char*)r_ref,
			    pointer_type->size[r_arch]));

    /* Receive the pointed data only if not already sent */
    if (gras_dd_is_r_null(r_ref, pointer_type->size[r_arch])) {
      VERB1("Not receiving data remotely referenced @%p since it's NULL",
	    *(void **)r_ref);
      *(void**)l_data = NULL;
      free(r_ref);
      break;
    }
         
    errcode = detect_cycle
            ? xbt_dict_get_ext(refs,
				(char*)r_ref, pointer_type->size[r_arch],
				(void**)&l_ref)
            : mismatch_error;

    if (errcode == mismatch_error) {
      int subsubcount = 0;
      void *l_referenced=NULL;

      VERB2("Receiving a ref to '%s', remotely @%p",
	    sub_type->name, *(void**)r_ref);
      if (sub_type->category_code == e_gras_datadesc_type_cat_array) {
	/* Damn. Reference to a dynamic array. Allocating the size for it 
	   is more complicated */
	gras_dd_cat_array_t array_data = sub_type->category.array_data;
	gras_datadesc_type_t subsub_type;

	subsubcount = array_data.fixed_size;
	if (subsubcount == 0)
	  TRYOLD(gras_dd_recv_int(sock, r_arch, &subsubcount));

	subsub_type = array_data.type;


	TRYOLD(gras_dd_alloc_ref(refs,
			      subsub_type->size[GRAS_THISARCH] * subsubcount, 
			      r_ref,pointer_type->size[r_arch], 
			      (char**)&l_referenced,
			      detect_cycle));
      } else {
	TRYOLD(gras_dd_alloc_ref(refs,sub_type->size[GRAS_THISARCH], 
			      r_ref,pointer_type->size[r_arch], 
			      (char**)&l_referenced,
			      detect_cycle));
      }

      TRYOLD(gras_datadesc_gen_recv_rec(sock,state,refs, sub_type,
				     r_arch,r_ref,pointer_type->size[r_arch],
				     (char*)l_referenced, subsubcount,
				     detect_cycle || sub_type->cycle));
				     
      *(void**)l_data=l_referenced;
      VERB3("'%s' remotely referenced at %p locally at %p",
	    sub_type->name, *(void**)r_ref, l_referenced);
      
    } else if (errcode == no_error) {
      VERB2("NOT receiving data remotely referenced @%p (already done, @%p here)",
	    *(void**)r_ref, *(void**)l_ref);

      *(void**)l_data=*l_ref;

    } else {
      return errcode;
    }
    free(r_ref);
    break;
  }

  case e_gras_datadesc_type_cat_array: {
    gras_dd_cat_array_t    array_data;
    int       count;
    char     *ptr;
    long int  elm_size;

    array_data = type->category.array_data;
    /* determine element count locally, or from caller, or from peer */
    count = array_data.fixed_size;
    if (count == 0)
      count = subsize;
    if (count == 0)
      TRYOLD(gras_dd_recv_int(sock, r_arch, &count));
    if (count == 0)
      RAISE1(mismatch_error,
	     "Invalid (=0) array size for type %s",type->name);

    /* receive the content */
    sub_type = array_data.type;
    if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
      VERB1("Array of %d scalars, get it in one shoot", count);
      if (sub_type->aligned_size[GRAS_THISARCH] >= 
	  sub_type->aligned_size[r_arch]) {
	TRYOLD(gras_trp_chunk_recv(sock, (char*)l_data, 
				sub_type->aligned_size[r_arch] * count));
	if (r_arch != GRAS_THISARCH)
	  TRYOLD(gras_dd_convert_elm(sub_type,count,r_arch, l_data,l_data));
      } else {
	ptr = xbt_malloc(sub_type->aligned_size[r_arch] * count);

	TRYOLD(gras_trp_chunk_recv(sock, (char*)ptr, 
				sub_type->size[r_arch] * count));
	if (r_arch != GRAS_THISARCH)
	  TRYOLD(gras_dd_convert_elm(sub_type,count,r_arch, ptr,l_data));
	free(ptr);
      }
    } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
	       sub_type->category.array_data.fixed_size > 0 &&
	       sub_type->category.array_data.type->category_code == e_gras_datadesc_type_cat_scalar) {
      gras_datadesc_type_t subsub_type;
      array_data = sub_type->category.array_data;
      subsub_type = array_data.type;
       
      VERB1("Array of %d fixed array of scalars, get it in one shot",count);
      if (subsub_type->aligned_size[GRAS_THISARCH] >= 
	  subsub_type->aligned_size[r_arch]) {
	TRYOLD(gras_trp_chunk_recv(sock, (char*)l_data, 
				subsub_type->aligned_size[r_arch] * count * 
				  array_data.fixed_size));
	if (r_arch != GRAS_THISARCH)
	  TRYOLD(gras_dd_convert_elm(subsub_type,count*array_data.fixed_size,r_arch, l_data,l_data));
      } else {
	ptr = xbt_malloc(subsub_type->aligned_size[r_arch] * count*array_data.fixed_size);

	TRYOLD(gras_trp_chunk_recv(sock, (char*)ptr, 
				subsub_type->size[r_arch] * count*array_data.fixed_size));
	if (r_arch != GRAS_THISARCH)
	  TRYOLD(gras_dd_convert_elm(subsub_type,count*array_data.fixed_size,r_arch, ptr,l_data));
	free(ptr);
      }
      
       
    } else {
      /* not scalar content, get it recursively (may contain pointers) */
      elm_size = sub_type->aligned_size[GRAS_THISARCH];
      VERB2("Receive a %d-long array of %s",count, sub_type->name);

      ptr = l_data;
      for (cpt=0; cpt<count; cpt++) {
	TRYOLD(gras_datadesc_gen_recv_rec(sock,state,refs, sub_type,
				       r_arch, NULL, 0, ptr,-1,
				       detect_cycle || sub_type->cycle));
				      
	ptr += elm_size;
      }
    }
    break;
  }
        
  default:
    xbt_assert0(0, "Invalid type");
  }
  
  if (type->recv)
    type->recv(type,state,l_data);

  return no_error;
}

/**
 * gras_datadesc_gen_recv:
 *
 * Get an instance of the datatype described by @type from the @socket, 
 * and store a pointer to it in @dst
 *
 */
xbt_error_t
gras_datadesc_gen_recv(gras_socket_t         sock, 
		       gras_datadesc_type_t  type,
		       int                   r_arch,
		       void                 *dst) {

  xbt_error_t errcode;
  gras_cbps_t  state; /* callback persistent state */
  xbt_dict_t  refs;  /* all references already sent */

  refs = xbt_dict_new();
  state = gras_cbps_new();

  printf("xbt_error_t gras_%s_recv(gras_socket_t sock,void *dst){\n",
	 type->name);
   
  errcode = gras_datadesc_gen_recv_rec(sock, state, refs, type, 
				       r_arch, NULL, 0,
				       (char *) dst,-1, 
				       sub_type->cycle); 

  printf("}\n");
  xbt_dict_free(&refs);
  gras_cbps_free(&state);

  return errcode;
}
#endif
