/* $Id$ */

/* ddt_new - creation/deletion of datatypes structs (private to this module)*/

/* Copyright (c) 2003 Olivier Aumage.                                       */
/* Copyright (c) 2003, 2004 Martin Quinson.                                 */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h" /* min()/max() */
#include "gras/DataDesc/datadesc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ddt_create,datadesc,"Creating new datadescriptions");

/**
 * gras_ddt_freev:
 *
 * gime that memory back, dude. I mean it.
 */
void gras_ddt_freev(void *ddt) {
  gras_datadesc_type_t type= (gras_datadesc_type_t)ddt;
  
  if (type) {
    gras_datadesc_free(&type);
  }
}

static gras_datadesc_type_t gras_ddt_new(const char *name) {
  gras_datadesc_type_t res;

  XBT_IN1("(%s)",name);
  res=xbt_new0(s_gras_datadesc_type_t,1);

  res->name = (char*)strdup(name);
  res->name_len = strlen(name);
  res->cycle = 0;
      
  xbt_set_add(gras_datadesc_set_local,
	       (xbt_set_elm_t)res,&gras_ddt_freev);
  XBT_OUT;
  return res;
}

/**
 * gras_datadesc_by_name:
 *
 * Retrieve a type from its name
 */
gras_datadesc_type_t gras_datadesc_by_name(const char *name) {

  gras_datadesc_type_t type;

  XBT_IN1("(%s)",name);
  if (xbt_set_get_by_name(gras_datadesc_set_local,
			   name,(xbt_set_elm_t*)&type) == no_error) {
    XBT_OUT;
    return type;
  } else { 
    XBT_OUT;
    return NULL;
  }
}

/**
 * gras_datadesc_by_id:
 *
 * Retrieve a type from its code
 */
xbt_error_t gras_datadesc_by_id(long int              code,
				 gras_datadesc_type_t *type) {
  XBT_IN;
  return xbt_set_get_by_id(gras_datadesc_set_local,
			    code,(xbt_set_elm_t*)type);
}

/**
 * gras_datadesc_scalar:
 *
 * Create a new scalar and give a pointer to it 
 */
gras_datadesc_type_t 
  gras_datadesc_scalar(const char                      *name,
		       gras_ddt_scalar_type_t           type,
		       enum e_gras_dd_scalar_encoding   encoding) {

  gras_datadesc_type_t res;
  long int arch;

  XBT_IN;
  res = gras_datadesc_by_name(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_scalar,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.scalar_data.encoding == encoding,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.scalar_data.type == type,
		 "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s",name);
    return res;
  }
  res = gras_ddt_new(name);

  for (arch = 0; arch < gras_arch_count; arch ++) {
#if 0
    long int sz;
    long int mask;
#endif
    res->size[arch]         = gras_arches[arch].sizeofs[type];
    res->alignment[arch]    = gras_arches[arch].boundaries[type];
    res->aligned_size[arch] = aligned(res->size[arch], res->alignment[arch]);
/* FIXME: kill the following after discution with Oli */
#if 0
    sz = res->size[arch];
    mask = sz;
    
    /* just in case you wonder, x>>1 == x/2 on all architectures when x>=0
       and a size is always>=0 */
    
    /* make sure mask have all the bits under the biggest one of size set to 1
       Example: size=000100101 => mask=0000111111 */
    while ((sz >>= 1)) {
      mask |= sz;
    } 
    
    if (res->size[arch] & (mask >> 1)) { /* if size have bits to one beside its biggest */
      /* size is not a power of 2 */
      /* alignment= next power of 2 after size */
      res->alignment[arch] = (res->size[arch] & ~(mask >> 1)) << 1;
      xbt_assert0(res->alignment[arch] != 0,
		   "scalar type too large");
      
      res->aligned_size[arch] = aligned(res->size[arch], res->alignment[arch]);
      xbt_assert0 (res->aligned_size[arch] >= 0,
		    "scalar type too large");
      
    } else {
      /* size is a power of 2, life is great */
      res->alignment[arch]       = res->size[arch];
      res->aligned_size[arch]    = res->size[arch];
    }
#endif     
  }

  res->category_code                 = e_gras_datadesc_type_cat_scalar;
  res->category.scalar_data.encoding = encoding;
  res->category.scalar_data.type     = type;
  XBT_OUT;
  
  return res;
}


/**
 * gras_dd_cat_field_free:
 *
 * Frees one struct or union field
 */
void gras_dd_cat_field_free(void *f) {
  gras_dd_cat_field_t field = *(gras_dd_cat_field_t *)f;
  XBT_IN;
  if (field) {
    if (field->name) 
      xbt_free(field->name);
    xbt_free(field);
  }
  XBT_OUT;
}

/**
 * gras_datadesc_struct:
 *
 * Create a new struct and give a pointer to it 
 */
gras_datadesc_type_t 
  gras_datadesc_struct(const char            *name) {

  gras_datadesc_type_t res;
  long int arch;
  
  XBT_IN1("(%s)",name);
  res = gras_datadesc_by_name(name);
  if (res) {
    /* FIXME: Check that field redefinition matches */
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_struct,
		 "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s",name);
    return res;
  }
  res = gras_ddt_new(name);

  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch] = 0;
    res->alignment[arch] = 0;
    res->aligned_size[arch] = 0;
  }
  res->category_code = e_gras_datadesc_type_cat_struct;
  res->category.struct_data.fields = 
       xbt_dynar_new(sizeof(gras_dd_cat_field_t),
		      &gras_dd_cat_field_free);

  XBT_OUT;
  return res;
}

/**
 * gras_datadesc_struct_append:
 *
 * Append a field to the struct
 */
void
gras_datadesc_struct_append(gras_datadesc_type_t struct_type,
			    const char          *name,
			    gras_datadesc_type_t field_type) {

  gras_dd_cat_field_t field;
  int arch;

  xbt_assert2(field_type,
	       "Cannot add the field '%s' into struct '%s': its type is NULL. Typo in get_by_name?",
	       name,struct_type->name);
  XBT_IN3("(%s %s.%s;)",field_type->name,struct_type->name,name);
  if (struct_type->category.struct_data.closed) {
    VERB1("Ignoring request to add field to struct %s (closed. Redefinition?)",
	  struct_type->name);
    return;
  }

  xbt_assert1(field_type->size != 0,
	       "Cannot add a dynamically sized field in structure %s",
	       struct_type->name);
    
  field=xbt_new(s_gras_dd_cat_field_t,1);
  field->name   = (char*)strdup(name);

  DEBUG0("----------------");
  DEBUG3("PRE s={size=%ld,align=%ld,asize=%ld}",
	 struct_type->size[GRAS_THISARCH], 
	 struct_type->alignment[GRAS_THISARCH], 
	 struct_type->aligned_size[GRAS_THISARCH]);
     
     
  for (arch=0; arch<gras_arch_count; arch ++) {
    field->offset[arch] = aligned(struct_type->size[arch],
				  field_type->alignment[arch]);

    struct_type->size[arch] = field->offset[arch] + field_type->size[arch];
    struct_type->alignment[arch] = max(struct_type->alignment[arch],
				       field_type->alignment[arch]);
    struct_type->aligned_size[arch] = aligned(struct_type->size[arch],
					      struct_type->alignment[arch]);
  }
  field->type   = field_type;
  field->pre    = NULL;
  field->post   = NULL;
  
  xbt_dynar_push(struct_type->category.struct_data.fields, &field);

  DEBUG3("Push a %s into %s at offset %ld.",
	 field_type->name, struct_type->name,field->offset[GRAS_THISARCH]);
  DEBUG3("  f={size=%ld,align=%ld,asize=%ld}",
	 field_type->size[GRAS_THISARCH], 
	 field_type->alignment[GRAS_THISARCH], 
	 field_type->aligned_size[GRAS_THISARCH]);
  DEBUG3("  s={size=%ld,align=%ld,asize=%ld}",
	 struct_type->size[GRAS_THISARCH], 
	 struct_type->alignment[GRAS_THISARCH], 
	 struct_type->aligned_size[GRAS_THISARCH]);
  XBT_OUT;
}

void
gras_datadesc_struct_close(gras_datadesc_type_t struct_type) {
  XBT_IN;
  struct_type->category.struct_data.closed = 1;
  DEBUG4("structure %s closed. size=%ld,align=%ld,asize=%ld",
	 struct_type->name,
	 struct_type->size[GRAS_THISARCH], 
	 struct_type->alignment[GRAS_THISARCH], 
	 struct_type->aligned_size[GRAS_THISARCH]);
}

/**
 * gras_datadesc_cycle_set:
 * 
 * Tell GRAS that the pointers of the type described by @ddt may present
 * some loop, and that the cycle detection mecanism is needed.
 *
 * Note that setting this option when not needed have a rather bad effect 
 * on the performance (several times slower on big data).
 */
void
gras_datadesc_cycle_set(gras_datadesc_type_t ddt) {
  ddt->cycle = 1;
}
/**
 * gras_datadesc_cycle_unset:
 * 
 * Tell GRAS that the pointers of the type described by @ddt do not present
 * any loop and that cycle detection mecanism are not needed.
 * (default)
 */
void
gras_datadesc_cycle_unset(gras_datadesc_type_t ddt) {
  ddt->cycle = 0;
}

/**
 * gras_datadesc_union:
 *
 * Create a new union and give a pointer to it 
 */
gras_datadesc_type_t 
gras_datadesc_union(const char                   *name,
		    gras_datadesc_type_cb_int_t   selector) {

  gras_datadesc_type_t res;
  int arch;

  XBT_IN1("(%s)",name);
  xbt_assert0(selector,
	       "Attempt to creat an union without field_count function");

  res = gras_datadesc_by_name(name);
  if (res) {
    /* FIXME: Check that field redefinition matches */
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_union,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.union_data.selector == selector,
		 "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s",name);
    return res;
  }

  res = gras_ddt_new(name);

  for (arch=0; arch<gras_arch_count; arch ++) {
     res->size[arch] = 0;
     res->alignment[arch] = 0;
     res->aligned_size[arch] = 0;
  }

  res->category_code		= e_gras_datadesc_type_cat_union;
  res->category.union_data.fields =
     xbt_dynar_new(sizeof(gras_dd_cat_field_t*),
		    &gras_dd_cat_field_free);
  res->category.union_data.selector = selector;

  return res;
}

/**
 * gras_datadesc_union_append:
 *
 * Append a field to the union
 */
void
gras_datadesc_union_append(gras_datadesc_type_t  union_type,
			   const char           *name,
			   gras_datadesc_type_t  field_type) {

  gras_dd_cat_field_t field;
  int arch;

  XBT_IN3("(%s %s.%s;)",field_type->name,union_type->name,name);
  xbt_assert1(field_type->size != 0,
	       "Cannot add a dynamically sized field in union %s",
	       union_type->name);

  if (union_type->category.union_data.closed) {
    VERB1("Ignoring request to add field to union %s (closed)",
	   union_type->name);
    return;
  }
    
  field=xbt_new0(s_gras_dd_cat_field_t,1);

  field->name   = (char*)strdup(name);
  field->type   = field_type;
  /* All offset are left to 0 in an union */
  
  xbt_dynar_push(union_type->category.union_data.fields, &field);

  for (arch=0; arch<gras_arch_count; arch ++) {
    union_type->size[arch] = max(union_type->size[arch],
				 field_type->size[arch]);
    union_type->alignment[arch] = max(union_type->alignment[arch],
				      field_type->alignment[arch]);
    union_type->aligned_size[arch] = aligned(union_type->size[arch],
					     union_type->alignment[arch]);
  }
}

void
gras_datadesc_union_close(gras_datadesc_type_t union_type) {
   union_type->category.union_data.closed = 1;
}
/**
 * gras_datadesc_ref:
 *
 * Create a new ref to a fixed type and give a pointer to it 
 */
gras_datadesc_type_t 
  gras_datadesc_ref(const char           *name,
		    gras_datadesc_type_t  referenced_type) {

  gras_datadesc_type_t res;
  gras_datadesc_type_t pointer_type = gras_datadesc_by_name("data pointer");
  int arch;

  XBT_IN1("(%s)",name);
  res = gras_datadesc_by_name(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_ref,
		 "Redefinition of %s does not match",name);
    xbt_assert1(res->category.ref_data.type == referenced_type,
		 "Redefinition of %s does not match",name);
    xbt_assert1(res->category.ref_data.selector == NULL,
		 "Redefinition of %s does not match",name);
    VERB1("Discarding redefinition of %s",name);
    return res;
  }

  res = gras_ddt_new(name);

  xbt_assert0(pointer_type, "Cannot get the description of data pointer");
      
  for (arch=0; arch<gras_arch_count; arch ++){
    res->size[arch] = pointer_type->size[arch];
    res->alignment[arch] = pointer_type->alignment[arch];
    res->aligned_size[arch] = pointer_type->aligned_size[arch];
  }
  
  res->category_code	 	  = e_gras_datadesc_type_cat_ref;
  res->category.ref_data.type     = referenced_type;
  res->category.ref_data.selector = NULL;

  return res;
}
/**
 * gras_datadesc_ref_generic:
 *
 * Create a new ref to a type given at use time, and give a pointer to it 
 */
gras_datadesc_type_t 
  gras_datadesc_ref_generic(const char                *name,
			    gras_datadesc_selector_t   selector) {

  gras_datadesc_type_t res;
  gras_datadesc_type_t pointer_type = gras_datadesc_by_name("data pointer");
  int arch;

  XBT_IN1("(%s)",name);
  res = gras_datadesc_by_name(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_ref,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.ref_data.type == NULL,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.ref_data.selector == selector,
		 "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s",name);
    return res;
  }
  res = gras_ddt_new(name);

  xbt_assert0(pointer_type, "Cannot get the description of data pointer");
      
  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch] = pointer_type->size[arch];
    res->alignment[arch] = pointer_type->alignment[arch];
    res->aligned_size[arch] = pointer_type->aligned_size[arch];
  }

  res->category_code		= e_gras_datadesc_type_cat_ref;

  res->category.ref_data.type     = NULL;
  res->category.ref_data.selector = selector;

  return res;
}

/**
 * gras_datadesc_array_fixed:
 *
 * Create a new array and give a pointer to it 
 */
gras_datadesc_type_t 
  gras_datadesc_array_fixed(const char           *name,
			    gras_datadesc_type_t  element_type,
			    long int              fixed_size) {

  gras_datadesc_type_t res;
  int arch;

  XBT_IN1("(%s)",name);
  res = gras_datadesc_by_name(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_array,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.type == element_type,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.fixed_size == fixed_size,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.dynamic_size == NULL,
		 "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s",name);

    return res;
  }
  res = gras_ddt_new(name);

  xbt_assert1(fixed_size > 0, "'%s' is a array of null fixed size",name);
  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch] = fixed_size * element_type->aligned_size[arch];
    res->alignment[arch] = element_type->alignment[arch];
    res->aligned_size[arch] = res->size[arch];
  }  

  res->category_code		= e_gras_datadesc_type_cat_array;

  res->category.array_data.type         = element_type;
  res->category.array_data.fixed_size   = fixed_size;
  res->category.array_data.dynamic_size = NULL;

  return res;
}
/**
 * gras_datadesc_array_dyn:
 *
 * Create a new array and give a pointer to it 
 */
gras_datadesc_type_t 
  gras_datadesc_array_dyn(const char                  *name,
			  gras_datadesc_type_t         element_type,
			  gras_datadesc_type_cb_int_t  dynamic_size) {

  gras_datadesc_type_t res;
  int arch;

  XBT_IN1("(%s)",name);
  xbt_assert1(dynamic_size,
	       "'%s' is a dynamic array without size discriminant",
	       name);

  res = gras_datadesc_by_name(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_array,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.type == element_type,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.fixed_size == 0,
		 "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.dynamic_size == dynamic_size,
		 "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s",name);

    return res;
  }

  res = gras_ddt_new(name);

  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch] = 0; /* make sure it indicates "dynamic" */
    res->alignment[arch] = element_type->alignment[arch];
    res->aligned_size[arch] = 0; /*FIXME: That was so in GS, but looks stupid*/
  }

  res->category_code		= e_gras_datadesc_type_cat_array;

  res->category.array_data.type         = element_type;
  res->category.array_data.fixed_size   = 0;
  res->category.array_data.dynamic_size = dynamic_size;

  return res;
}

/**
 * gras_datadesc_ref_pop_arr:
 *
 * Most of the time, you want to include a reference in your structure which
 * is a pointer to a dynamic array whose size is fixed by another field of 
 * your structure.
 *
 * This case pops up so often that this function was created to take care of
 * this case. It creates a dynamic array type whose size is poped from the 
 * current cbps, and then create a reference to it.
 *
 * The name of the created datatype will be the name of the element type, with
 * '[]*' appended to it.
 *
 * Then to use it, you just have to make sure that your structure pre-callback
 * does push the size of the array in the cbps (using #gras_cbps_i_push), and 
 * you are set. 
 *
 * But be remember that this is a stack. If you have two different pop_arr, you
 * should push the second one first, so that the first one is on the top of the 
 * list when the first field gets transfered.
 *
 */
gras_datadesc_type_t 
  gras_datadesc_ref_pop_arr(gras_datadesc_type_t element_type) {

  gras_datadesc_type_t res;
  char *name=(char*)xbt_malloc(strlen(element_type->name) + 4);

  sprintf(name,"%s[]",element_type->name);

  res = gras_datadesc_array_dyn(name,element_type,
				gras_datadesc_cb_pop);

  sprintf(name,"%s[]*",element_type->name);
  res = gras_datadesc_ref(name,res);

  xbt_free(name);

  return res;
}
xbt_error_t
gras_datadesc_import_nws(const char           *name,
			 const DataDescriptor *desc,
			 unsigned long         howmany,
	       /* OUT */ gras_datadesc_type_t *dst) {
  RAISE_UNIMPLEMENTED;
}

/**
 * gras_datadesc_cb_send:
 *
 * Add a pre-send callback to this datadesc.
 * (useful to push the sizes of the upcoming arrays, for example)
 */
void gras_datadesc_cb_send (gras_datadesc_type_t          type,
			    gras_datadesc_type_cb_void_t  send) {
  type->send = send;
}
/**
 * gras_datadesc_cb_recv:
 *
 * Add a post-receive callback to this datadesc.
 * (useful to put the function pointers to the rigth value, for example)
 */
void gras_datadesc_cb_recv(gras_datadesc_type_t          type,
			   gras_datadesc_type_cb_void_t  recv) {
  type->recv = recv;
}
/**
 * gras_dd_find_field:
 * 
 * Returns the type descriptor of the given field. Abort on error.
 */
static gras_datadesc_type_t 
  gras_dd_find_field(gras_datadesc_type_t  type,
		     const char           *field_name) {
   xbt_dynar_t         field_array;
   
   gras_dd_cat_field_t  field=NULL;
   int                  field_num;
   
   if (type->category_code == e_gras_datadesc_type_cat_union) {
      field_array = type->category.union_data.fields;
   } else if (type->category_code == e_gras_datadesc_type_cat_struct) {
      field_array = type->category.struct_data.fields;
   } else {
      ERROR2("%s (%p) is not a struct nor an union. There is no field.", type->name,(void*)type);
      xbt_abort();
   }
   xbt_dynar_foreach(field_array,field_num,field) {
      if (!strcmp(field_name,field->name)) {
	 return field->type;
      }
   }
   ERROR2("No field nammed %s in %s",field_name,type->name);
   xbt_abort();

}
				  
/**
 * gras_datadesc_cb_field_send:
 *
 * Add a pre-send callback to the given field of the datadesc (which must be a struct or union).
 * (useful to push the sizes of the upcoming arrays, for example)
 */
void gras_datadesc_cb_field_send (gras_datadesc_type_t          type,
				  const char                   *field_name,
				  gras_datadesc_type_cb_void_t  send) {
   
   gras_datadesc_type_t sub_type=gras_dd_find_field(type,field_name);   
   sub_type->send = send;
}

/**
 * gras_datadesc_cb_field_push:
 *
 * Add a pre-send callback to the given field resulting in its value to be pushed to
 * the stack of sizes. It must be a int, unsigned int, long int or unsigned long int.
 */
void gras_datadesc_cb_field_push (gras_datadesc_type_t  type,
				  const char           *field_name) {
   
   gras_datadesc_type_t sub_type=gras_dd_find_field(type,field_name);
   if (!strcmp("int",sub_type->name)) {
      sub_type->send = gras_datadesc_cb_push_int;
   } else if (!strcmp("unsigned int",sub_type->name)) {
      sub_type->send = gras_datadesc_cb_push_uint;
   } else if (!strcmp("long int",sub_type->name)) {
      sub_type->send = gras_datadesc_cb_push_lint;
   } else if (!strcmp("unsigned long int",sub_type->name)) {
      sub_type->send = gras_datadesc_cb_push_ulint;
   } else {
      ERROR1("Field %s is not an int, unsigned int, long int neither unsigned long int",
	     sub_type->name);
      xbt_abort();
   }
}
/**
 * gras_datadesc_cb_field_recv:
 *
 * Add a post-receive callback to the given field of the datadesc (which must be a struct or union).
 * (useful to put the function pointers to the right value, for example)
 */
void gras_datadesc_cb_field_recv(gras_datadesc_type_t          type,
				 const char                   *field_name,
				 gras_datadesc_type_cb_void_t  recv) {
   
   gras_datadesc_type_t sub_type=gras_dd_find_field(type,field_name);   
   sub_type->recv = recv;
}

/**
 * gras_datadesc_free:
 *
 * Free a datadesc. Should only be called at xbt_exit. 
 */
void gras_datadesc_free(gras_datadesc_type_t *type) {

  DEBUG1("Let's free ddt %s",(*type)->name);
  
  switch ((*type)->category_code) {
  case e_gras_datadesc_type_cat_scalar:
  case e_gras_datadesc_type_cat_ref:
  case e_gras_datadesc_type_cat_array:
    /* nothing to free in there */
    break;
    
  case e_gras_datadesc_type_cat_ignored:
    if ((*type)->category.ignored_data.free_func) {
      (*type)->category.ignored_data.free_func
	((*type)->category.ignored_data.default_value);
    }
    break;
    
  case e_gras_datadesc_type_cat_struct:
    xbt_dynar_free(&( (*type)->category.struct_data.fields ));
    break;
    
  case e_gras_datadesc_type_cat_union:
    xbt_dynar_free(&( (*type)->category.union_data.fields ));
    break;
    
  default:
    /* datadesc was invalid. Killing it is like euthanasy, I guess */
    break;
  }
  xbt_free((*type)->name);
  xbt_free(*type);
  type=NULL;
}
