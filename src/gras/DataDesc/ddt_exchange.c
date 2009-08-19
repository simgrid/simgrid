/* $Id$ */

/* ddt_exchange - send/recv data described                                  */

/* Copyright (c) 2003-2009 The SimGrid Team.  All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/DataDesc/datadesc_private.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_send/recv */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_ddt_exchange, gras_ddt,
                                "Sending data over the network");
const char *gras_datadesc_cat_names[9] = {
  "undefined",
  "scalar", "struct", "union", "ref", "array", "ignored",
  "invalid"
};

static gras_datadesc_type_t int_type = NULL;
static gras_datadesc_type_t pointer_type = NULL;

static XBT_INLINE void
gras_dd_send_int(gras_socket_t sock, int *i, int stable)
{

  if (!int_type) {
    int_type = gras_datadesc_by_name("int");
    xbt_assert(int_type);
  }

  DEBUG1("send_int(%u)", *i);
  gras_trp_send(sock, (char *) i, int_type->size[GRAS_THISARCH], stable);
}

static XBT_INLINE void
gras_dd_recv_int(gras_socket_t sock, int r_arch, int *i)
{

  if (!int_type) {
    int_type = gras_datadesc_by_name("int");
    xbt_assert(int_type);
  }

  if (int_type->size[GRAS_THISARCH] >= int_type->size[r_arch]) {
    gras_trp_recv(sock, (char *) i, int_type->size[r_arch]);
    if (r_arch != GRAS_THISARCH)
      gras_dd_convert_elm(int_type, 1, r_arch, i, i);
  } else {
    void *ptr = xbt_malloc(int_type->size[r_arch]);

    gras_trp_recv(sock, (char *) ptr, int_type->size[r_arch]);
    if (r_arch != GRAS_THISARCH)
      gras_dd_convert_elm(int_type, 1, r_arch, ptr, i);
    free(ptr);
  }
  DEBUG1("recv_int(%u)", *i);
}

/*
 * Note: here we suppose that the remote NULL is a sequence
 *       of 'length' bytes set to 0.
 * FIXME: Check in configure?
 */
static XBT_INLINE int gras_dd_is_r_null(char **r_ptr, long int length)
{
  int i;

  for (i = 0; i < length; i++) {
    if (((unsigned char *) r_ptr)[i]) {
      return 0;
    }
  }

  return 1;
}

static XBT_INLINE void gras_dd_alloc_ref(xbt_dict_t refs, long int size, char **r_ref, long int r_len,  /* pointer_type->size[r_arch] */
                                         char **l_ref, int detect_cycle)
{
  char *l_data = NULL;

  xbt_assert1(size > 0, "Cannot allocate %ld bytes!", size);
  l_data = xbt_malloc((size_t) size);

  *l_ref = l_data;
  DEBUG5("alloc_ref: l_data=%p, &l_data=%p; r_ref=%p; *r_ref=%p, r_len=%ld",
         (void *) l_data, (void *) &l_data,
         (void *) r_ref, (void *) (r_ref ? *r_ref : NULL), r_len);
  if (detect_cycle && r_ref && !gras_dd_is_r_null(r_ref, r_len)) {
    void *ptr = xbt_malloc(sizeof(void *));

    memcpy(ptr, l_ref, sizeof(void *));

    DEBUG2("Insert l_ref=%p under r_ref=%p", *(void **) ptr,
           *(void **) r_ref);

    if (detect_cycle)
      xbt_dict_set_ext(refs, (const char *) r_ref, r_len, ptr, xbt_free_f);
  }
}

static int
gras_datadesc_memcpy_rec(gras_cbps_t state,
                         xbt_dict_t refs,
                         gras_datadesc_type_t type,
                         char *src, char *dst, int subsize, int detect_cycle)
{


  unsigned int cpt;
  gras_datadesc_type_t sub_type;        /* type on which we recurse */
  int count = 0;

  VERB5("Copy a %s (%s) from %p to %p (local sizeof=%ld)",
        type->name, gras_datadesc_cat_names[type->category_code],
        src, dst, type->size[GRAS_THISARCH]);

  if (type->send) {
    type->send(type, state, src);
  }

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    memcpy(dst, src, type->size[GRAS_THISARCH]);
    count += type->size[GRAS_THISARCH];
    break;

  case e_gras_datadesc_type_cat_struct:{
      gras_dd_cat_struct_t struct_data;
      gras_dd_cat_field_t field;
      char *field_src;
      char *field_dst;

      struct_data = type->category.struct_data;
      xbt_assert1(struct_data.closed,
                  "Please call gras_datadesc_declare_struct_close on %s before copying it",
                  type->name);
      VERB1(">> Copy all fields of the structure %s", type->name);
      xbt_dynar_foreach(struct_data.fields, cpt, field) {
        field_src = src + field->offset[GRAS_THISARCH];
        field_dst = dst + field->offset[GRAS_THISARCH];

        sub_type = field->type;

        if (field->send)
          field->send(type, state, field_src);

        DEBUG1("Copy field %s", field->name);
        count +=
          gras_datadesc_memcpy_rec(state, refs, sub_type, field_src,
                                   field_dst, 0, detect_cycle
                                   || sub_type->cycle);

        if (XBT_LOG_ISENABLED(gras_ddt_exchange, xbt_log_priority_verbose)) {
          if (sub_type == gras_datadesc_by_name("unsigned int")) {
            VERB2("Copied value for field '%s': %d (type: unsigned int)",
                  field->name, *(unsigned int *) field_dst);
          } else if (sub_type == gras_datadesc_by_name("int")) {
            VERB2("Copied value for field '%s': %d (type: int)", field->name,
                  *(int *) field_dst);

          } else if (sub_type == gras_datadesc_by_name("unsigned long int")) {
            VERB2
              ("Copied value for field '%s': %ld (type: unsigned long int)",
               field->name, *(unsigned long int *) field_dst);
          } else if (sub_type == gras_datadesc_by_name("long int")) {
            VERB2("Copied value for field '%s': %ld (type: long int)",
                  field->name, *(long int *) field_dst);

          } else if (sub_type == gras_datadesc_by_name("string")) {
            VERB2("Copied value for field '%s': '%s' (type: string)",
                  field->name, *(char **) field_dst);
          } else {
            VERB1("Copied a value for field '%s' (type not scalar?)",
                  field->name);
          }
        }

      }
      VERB1("<< Copied all fields of the structure %s", type->name);

      break;
    }

  case e_gras_datadesc_type_cat_union:{
      gras_dd_cat_union_t union_data;
      gras_dd_cat_field_t field = NULL;
      unsigned int field_num;

      union_data = type->category.union_data;

      xbt_assert1(union_data.closed,
                  "Please call gras_datadesc_declare_union_close on %s before copying it",
                  type->name);
      /* retrieve the field number */
      field_num = union_data.selector(type, state, src);

      xbt_assert1(field_num > 0,
                  "union field selector of %s gave a negative value",
                  type->name);

      xbt_assert3(field_num < xbt_dynar_length(union_data.fields),
                  "union field selector of %s returned %d but there is only %lu fields",
                  type->name, field_num, xbt_dynar_length(union_data.fields));

      /* Copy the content */
      field =
        xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
      sub_type = field->type;

      if (field->send)
        field->send(type, state, src);

      count += gras_datadesc_memcpy_rec(state, refs, sub_type, src, dst, 0,
                                        detect_cycle || sub_type->cycle);

      break;
    }

  case e_gras_datadesc_type_cat_ref:{
      gras_dd_cat_ref_t ref_data;
      char **o_ref = NULL;
      char **n_ref = NULL;
      int reference_is_to_cpy;

      ref_data = type->category.ref_data;

      /* Detect the referenced type */
      sub_type = ref_data.type;
      if (sub_type == NULL) {
        sub_type = (*ref_data.selector) (type, state, src);
      }

      /* Send the pointed data only if not already sent */
      if (*(void **) src == NULL) {
        VERB0("Not copying NULL referenced data");
        *(void **) dst = NULL;
        break;
      }
      o_ref = (char **) src;

      reference_is_to_cpy = 1;
      if (detect_cycle &&
           (n_ref=xbt_dict_get_or_null_ext(refs, (char *) o_ref, sizeof(char *))))
        /* already known, no need to copy it */
        reference_is_to_cpy = 0;

      if (reference_is_to_cpy) {
        int subsubcount = -1;
        void *l_referenced = NULL;
        VERB2("Copy a ref to '%s' referenced at %p", sub_type->name,
              (void *) *o_ref);

        if (!pointer_type) {
          pointer_type = gras_datadesc_by_name("data pointer");
          xbt_assert(pointer_type);
        }

        if (sub_type->category_code == e_gras_datadesc_type_cat_array) {
          /* Damn. Reference to a dynamic array. Allocating the space for it is more complicated */
          gras_dd_cat_array_t array_data = sub_type->category.array_data;
          gras_datadesc_type_t subsub_type;

          subsub_type = array_data.type;
          subsubcount = array_data.fixed_size;
          if (subsubcount == -1)
            subsubcount = array_data.dynamic_size(subsub_type, state, *o_ref);

          if (subsubcount != 0)
            gras_dd_alloc_ref(refs,
                              subsub_type->size[GRAS_THISARCH] * subsubcount,
                              o_ref, pointer_type->size[GRAS_THISARCH],
                              (char **) &l_referenced, detect_cycle);
        } else {
          gras_dd_alloc_ref(refs, sub_type->size[GRAS_THISARCH],
                            o_ref, pointer_type->size[GRAS_THISARCH],
                            (char **) &l_referenced, detect_cycle);
        }

        count += gras_datadesc_memcpy_rec(state, refs, sub_type,
                                          *o_ref, (char *) l_referenced,
                                          subsubcount, detect_cycle
                                          || sub_type->cycle);

        *(void **) dst = l_referenced;
        VERB3("'%s' previously referenced at %p now at %p",
              sub_type->name, *(void **) o_ref, l_referenced);

      } else {
        VERB2
          ("NOT copying data previously referenced @%p (already done, @%p now)",
           *(void **) o_ref, *(void **) n_ref);

        *(void **) dst = *n_ref;

      }
      break;
    }

  case e_gras_datadesc_type_cat_array:{
      gras_dd_cat_array_t array_data;
      unsigned long int array_count;
      char *src_ptr = src;
      char *dst_ptr = dst;
      long int elm_size;

      array_data = type->category.array_data;

      /* determine and send the element count */
      array_count = array_data.fixed_size;
      if (array_count == -1)
        array_count = subsize;
      if (array_count == -1) {
        array_count = array_data.dynamic_size(type, state, src);
        xbt_assert1(array_count >= 0,
                    "Invalid (negative) array size for type %s", type->name);
      }

      /* send the content */
      sub_type = array_data.type;
      elm_size = sub_type->aligned_size[GRAS_THISARCH];
      if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
        VERB1("Array of %ld scalars, copy it in one shot", array_count);
        memcpy(dst, src, sub_type->aligned_size[GRAS_THISARCH] * array_count);
        count += sub_type->aligned_size[GRAS_THISARCH] * array_count;
      } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
                 sub_type->category.array_data.fixed_size > 0 &&
                 sub_type->category.array_data.type->category_code ==
                 e_gras_datadesc_type_cat_scalar) {

        VERB1("Array of %ld fixed array of scalars, copy it in one shot",
              array_count);
        memcpy(dst, src,
               sub_type->category.array_data.type->aligned_size[GRAS_THISARCH]
               * array_count * sub_type->category.array_data.fixed_size);
        count +=
          sub_type->category.array_data.type->aligned_size[GRAS_THISARCH]
          * array_count * sub_type->category.array_data.fixed_size;

      } else {
        VERB1("Array of %ld stuff, copy it in one after the other",
              array_count);
        for (cpt = 0; cpt < array_count; cpt++) {
          VERB2("Copy the %dth stuff out of %ld", cpt, array_count);
          count +=
            gras_datadesc_memcpy_rec(state, refs, sub_type, src_ptr, dst_ptr,
                                     0, detect_cycle || sub_type->cycle);
          src_ptr += elm_size;
          dst_ptr += elm_size;
        }
      }
      break;
    }

  default:
    xbt_die("Invalid type");
  }

  return count;
}

/**
 * gras_datadesc_memcpy:
 *
 * Copy the data pointed by src and described by type
 * to a new location, and store a pointer to it in dst.
 *
 */
int gras_datadesc_memcpy(gras_datadesc_type_t type, void *src, void *dst)
{
  xbt_ex_t e;
  static gras_cbps_t state = NULL;
  static xbt_dict_t refs = NULL;        /* all references already sent */
  int size = 0;

  xbt_assert0(type, "called with NULL type descriptor");

  DEBUG3("Memcopy a %s from %p to %p", gras_datadesc_get_name(type), src,
         dst);
  if (!state) {
    state = gras_cbps_new();
    refs = xbt_dict_new();
  }

  TRY {
    size =
      gras_datadesc_memcpy_rec(state, refs, type, (char *) src, (char *) dst,
                               0, type->cycle);
  } CLEANUP {
    xbt_dict_reset(refs);
    gras_cbps_reset(state);
  } CATCH(e) {
    RETHROW;
  }
  return size;
}

/***
 *** Direct use functions
 ***/

static void
gras_datadesc_send_rec(gras_socket_t sock,
                       gras_cbps_t state,
                       xbt_dict_t refs,
                       gras_datadesc_type_t type,
                       char *data, int detect_cycle)
{

  unsigned int cpt;
  gras_datadesc_type_t sub_type;        /* type on which we recurse */

  VERB2("Send a %s (%s)",
        type->name, gras_datadesc_cat_names[type->category_code]);

  if (!strcmp(type->name, "string"))
    VERB1("value: '%s'", *(char **) data);

  if (type->send) {
    type->send(type, state, data);
    DEBUG0("Run the emission callback");
  }

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    gras_trp_send(sock, data, type->size[GRAS_THISARCH], 1);
    break;

  case e_gras_datadesc_type_cat_struct:{
      gras_dd_cat_struct_t struct_data;
      gras_dd_cat_field_t field;
      char *field_data;

      struct_data = type->category.struct_data;
      xbt_assert1(struct_data.closed,
                  "Please call gras_datadesc_declare_struct_close on %s before sending it",
                  type->name);
      VERB1(">> Send all fields of the structure %s", type->name);
      xbt_dynar_foreach(struct_data.fields, cpt, field) {
        field_data = data;
        field_data += field->offset[GRAS_THISARCH];

        sub_type = field->type;

        if (field->send) {
          DEBUG1("Run the emission callback of field %s", field->name);
          field->send(type, state, field_data);
        }

        VERB1("Send field %s", field->name);
        gras_datadesc_send_rec(sock, state, refs, sub_type, field_data,
                               detect_cycle || sub_type->cycle);

      }
      VERB1("<< Sent all fields of the structure %s", type->name);

      break;
    }

  case e_gras_datadesc_type_cat_union:{
      gras_dd_cat_union_t union_data;
      gras_dd_cat_field_t field = NULL;
      int field_num;

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
      gras_dd_send_int(sock, &field_num, 0 /* not stable */ );

      /* Send the content */
      field =
        xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
      sub_type = field->type;

      if (field->send)
        field->send(type, state, data);

      gras_datadesc_send_rec(sock, state, refs, sub_type, data,
                             detect_cycle || sub_type->cycle);

      break;
    }

  case e_gras_datadesc_type_cat_ref:{
      gras_dd_cat_ref_t ref_data;
      void **ref = (void **) data;
      int reference_is_to_send;

      ref_data = type->category.ref_data;

      /* Detect the referenced type and send it to peer if needed */
      sub_type = ref_data.type;
      if (sub_type == NULL) {
        sub_type = (*ref_data.selector) (type, state, data);
        gras_dd_send_int(sock, &(sub_type->code), 1 /*stable */ );
      }

      /* Send the actual value of the pointer for cycle handling */
      if (!pointer_type) {
        pointer_type = gras_datadesc_by_name("data pointer");
        xbt_assert(pointer_type);
      }

      gras_trp_send(sock, (char *) data,
                    pointer_type->size[GRAS_THISARCH], 1 /*stable */ );

      /* Send the pointed data only if not already sent */
      if (*(void **) data == NULL) {
        VERB0("Not sending NULL referenced data");
        break;
      }

      reference_is_to_send = 1;
      /* return ignored. Just checking whether it's known or not */
      if (detect_cycle && xbt_dict_get_or_null_ext(refs, (char *) ref, sizeof(char *)))
        reference_is_to_send = 0;

      if (reference_is_to_send) {
        VERB1("Sending data referenced at %p", (void *) *ref);
        if (detect_cycle)
          xbt_dict_set_ext(refs, (char *) ref, sizeof(void *), ref, NULL);
        gras_datadesc_send_rec(sock, state, refs, sub_type, *ref,
                               detect_cycle || sub_type->cycle);

      } else {
        VERB1("Not sending data referenced at %p (already done)",
              (void *) *ref);
      }

      break;
    }

  case e_gras_datadesc_type_cat_array:{
      gras_dd_cat_array_t array_data;
      int count;
      char *ptr = data;
      long int elm_size;

      array_data = type->category.array_data;

      /* determine and send the element count */
      count = array_data.fixed_size;
      if (count == -1) {
        count = array_data.dynamic_size(type, state, data);
        xbt_assert1(count >= 0,
                    "Invalid (negative) array size for type %s", type->name);
        gras_dd_send_int(sock, &count, 0 /*non-stable */ );
      }

      /* send the content */
      sub_type = array_data.type;
      elm_size = sub_type->aligned_size[GRAS_THISARCH];
      if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
        VERB1("Array of %d scalars, send it in one shot", count);
        gras_trp_send(sock, data,
                      sub_type->aligned_size[GRAS_THISARCH] * count,
                      0 /* not stable */ );
      } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
                 sub_type->category.array_data.fixed_size > 0 &&
                 sub_type->category.array_data.type->category_code ==
                 e_gras_datadesc_type_cat_scalar) {

        VERB1("Array of %d fixed array of scalars, send it in one shot",
              count);
        gras_trp_send(sock, data,
                      sub_type->category.array_data.
                      type->aligned_size[GRAS_THISARCH]
                      * count * sub_type->category.array_data.fixed_size,
                      0 /* not stable */ );

      } else {
        for (cpt = 0; cpt < count; cpt++) {
          gras_datadesc_send_rec(sock, state, refs, sub_type, ptr,
                                 detect_cycle || sub_type->cycle);
          ptr += elm_size;
        }
      }
      break;
    }

  default:
    xbt_die("Invalid type");
  }
}

/**
 * gras_datadesc_send:
 *
 * Copy the data pointed by src and described by type to the socket
 *
 */
void gras_datadesc_send(gras_socket_t sock,
                        gras_datadesc_type_t type, void *src)
{

  xbt_ex_t e;
  static gras_cbps_t state = NULL;
  static xbt_dict_t refs = NULL;        /* all references already sent */

  xbt_assert0(type, "called with NULL type descriptor");

  if (!state) {
    state = gras_cbps_new();
    refs = xbt_dict_new();
  }

  TRY {
    gras_datadesc_send_rec(sock, state, refs, type, (char *) src,
                           type->cycle);
  } CLEANUP {
    xbt_dict_reset(refs);
    gras_cbps_reset(state);
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
gras_datadesc_recv_rec(gras_socket_t sock,
                       gras_cbps_t state,
                       xbt_dict_t refs,
                       gras_datadesc_type_t type,
                       int r_arch,
                       char **r_data,
                       long int r_lgr,
                       char *l_data, int subsize, int detect_cycle)
{

  unsigned int cpt;
  gras_datadesc_type_t sub_type;

  VERB2("Recv a %s @%p", type->name, (void *) l_data);
  xbt_assert(l_data);

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    if (type->size[GRAS_THISARCH] == type->size[r_arch]) {
      gras_trp_recv(sock, (char *) l_data, type->size[r_arch]);
      if (r_arch != GRAS_THISARCH)
        gras_dd_convert_elm(type, 1, r_arch, l_data, l_data);
    } else {
      void *ptr = xbt_malloc(type->size[r_arch]);

      gras_trp_recv(sock, (char *) ptr, type->size[r_arch]);
      if (r_arch != GRAS_THISARCH)
        gras_dd_convert_elm(type, 1, r_arch, ptr, l_data);
      free(ptr);
    }
    break;

  case e_gras_datadesc_type_cat_struct:{
      gras_dd_cat_struct_t struct_data;
      gras_dd_cat_field_t field;

      struct_data = type->category.struct_data;

      xbt_assert1(struct_data.closed,
                  "Please call gras_datadesc_declare_struct_close on %s before receiving it",
                  type->name);
      VERB1(">> Receive all fields of the structure %s", type->name);
      xbt_dynar_foreach(struct_data.fields, cpt, field) {
        char *field_data = l_data + field->offset[GRAS_THISARCH];

        sub_type = field->type;

        gras_datadesc_recv_rec(sock, state, refs, sub_type,
                               r_arch, NULL, 0,
                               field_data, -1,
                               detect_cycle || sub_type->cycle);

        if (field->recv) {
          DEBUG1("Run the reception callback of field %s", field->name);
          field->recv(type, state, (void *) l_data);
        }

      }
      VERB1("<< Received all fields of the structure %s", type->name);

      break;
    }

  case e_gras_datadesc_type_cat_union:{
      gras_dd_cat_union_t union_data;
      gras_dd_cat_field_t field = NULL;
      int field_num;

      union_data = type->category.union_data;

      xbt_assert1(union_data.closed,
                  "Please call gras_datadesc_declare_union_close on %s before receiving it",
                  type->name);
      /* retrieve the field number */
      gras_dd_recv_int(sock, r_arch, &field_num);
      if (field_num < 0)
        THROW1(mismatch_error, 0,
               "Received union field for %s is negative", type->name);
      if (field_num > xbt_dynar_length(union_data.fields))
        THROW3(mismatch_error, 0,
               "Received union field for %s is said to be #%d but there is only %lu fields",
               type->name, field_num, xbt_dynar_length(union_data.fields));

      /* Recv the content */
      field =
        xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
      sub_type = field->type;

      gras_datadesc_recv_rec(sock, state, refs, sub_type,
                             r_arch, NULL, 0,
                             l_data, -1, detect_cycle || sub_type->cycle);
      if (field->recv)
        field->recv(type, state, l_data);

      break;
    }

  case e_gras_datadesc_type_cat_ref:{
      char **r_ref = NULL;
      char **l_ref = NULL;
      gras_dd_cat_ref_t ref_data;
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

      gras_trp_recv(sock, (char *) r_ref, pointer_type->size[r_arch]);

      /* Receive the pointed data only if not already sent */
      if (gras_dd_is_r_null(r_ref, pointer_type->size[r_arch])) {
        VERB1("Not receiving data remotely referenced @%p since it's NULL",
              *(void **) r_ref);
        *(void **) l_data = NULL;
        free(r_ref);
        break;
      }

      reference_is_to_recv = 1;
      if (detect_cycle && (l_ref =
            xbt_dict_get_or_null_ext(refs, (char *) r_ref,
                             pointer_type->size[r_arch])))
        reference_is_to_recv = 0;

      if (reference_is_to_recv) {
        int subsubcount = -1;
        void *l_referenced = NULL;

        VERB2("Receiving a ref to '%s', remotely @%p",
              sub_type->name, *(void **) r_ref);
        if (sub_type->category_code == e_gras_datadesc_type_cat_array) {
          /* Damn. Reference to a dynamic array. Allocating the space for it is more complicated */
          gras_dd_cat_array_t array_data = sub_type->category.array_data;
          gras_datadesc_type_t subsub_type;

          subsubcount = array_data.fixed_size;
          if (subsubcount == -1)
            gras_dd_recv_int(sock, r_arch, &subsubcount);

          subsub_type = array_data.type;

          if (subsubcount != 0)
            gras_dd_alloc_ref(refs,
                              subsub_type->size[GRAS_THISARCH] * subsubcount,
                              r_ref, pointer_type->size[r_arch],
                              (char **) &l_referenced, detect_cycle);
          else
            l_referenced = NULL;
        } else {
          gras_dd_alloc_ref(refs, sub_type->size[GRAS_THISARCH],
                            r_ref, pointer_type->size[r_arch],
                            (char **) &l_referenced, detect_cycle);
        }

        if (l_referenced != NULL)
          gras_datadesc_recv_rec(sock, state, refs, sub_type,
                                 r_arch, r_ref, pointer_type->size[r_arch],
                                 (char *) l_referenced, subsubcount,
                                 detect_cycle || sub_type->cycle);

        *(void **) l_data = l_referenced;
        VERB3("'%s' remotely referenced at %p locally at %p",
              sub_type->name, *(void **) r_ref, l_referenced);

      } else {
        VERB2
          ("NOT receiving data remotely referenced @%p (already done, @%p here)",
           *(void **) r_ref, *(void **) l_ref);

        *(void **) l_data = *l_ref;

      }
      free(r_ref);
      break;
    }

  case e_gras_datadesc_type_cat_array:{
      gras_dd_cat_array_t array_data;
      int count;
      char *ptr;
      long int elm_size;

      array_data = type->category.array_data;
      /* determine element count locally, or from caller, or from peer */
      count = array_data.fixed_size;
      if (count == -1)
        count = subsize;
      if (count == -1)
        gras_dd_recv_int(sock, r_arch, &count);
      if (count == -1)
        THROW1(mismatch_error, 0,
               "Invalid (=-1) array size for type %s", type->name);

      /* receive the content */
      sub_type = array_data.type;
      if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
        VERB1("Array of %d scalars, get it in one shoot", count);
        if (sub_type->aligned_size[GRAS_THISARCH] >=
            sub_type->aligned_size[r_arch]) {
          gras_trp_recv(sock, (char *) l_data,
                        sub_type->aligned_size[r_arch] * count);
          if (r_arch != GRAS_THISARCH)
            gras_dd_convert_elm(sub_type, count, r_arch, l_data, l_data);
        } else {
          ptr = xbt_malloc(sub_type->aligned_size[r_arch] * count);

          gras_trp_recv(sock, (char *) ptr, sub_type->size[r_arch] * count);
          if (r_arch != GRAS_THISARCH)
            gras_dd_convert_elm(sub_type, count, r_arch, ptr, l_data);
          free(ptr);
        }
      } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
                 sub_type->category.array_data.fixed_size >= 0 &&
                 sub_type->category.array_data.type->category_code ==
                 e_gras_datadesc_type_cat_scalar) {
        gras_datadesc_type_t subsub_type;
        array_data = sub_type->category.array_data;
        subsub_type = array_data.type;

        VERB1("Array of %d fixed array of scalars, get it in one shot",
              count);
        if (subsub_type->aligned_size[GRAS_THISARCH] >=
            subsub_type->aligned_size[r_arch]) {
          gras_trp_recv(sock, (char *) l_data,
                        subsub_type->aligned_size[r_arch] * count *
                        array_data.fixed_size);
          if (r_arch != GRAS_THISARCH)
            gras_dd_convert_elm(subsub_type, count * array_data.fixed_size,
                                r_arch, l_data, l_data);
        } else {
          ptr =
            xbt_malloc(subsub_type->aligned_size[r_arch] * count *
                       array_data.fixed_size);

          gras_trp_recv(sock, (char *) ptr,
                        subsub_type->size[r_arch] * count *
                        array_data.fixed_size);
          if (r_arch != GRAS_THISARCH)
            gras_dd_convert_elm(subsub_type, count * array_data.fixed_size,
                                r_arch, ptr, l_data);
          free(ptr);
        }


      } else {
        /* not scalar content, get it recursively (may contain pointers) */
        elm_size = sub_type->aligned_size[GRAS_THISARCH];
        VERB2("Receive a %d-long array of %s", count, sub_type->name);

        ptr = l_data;
        for (cpt = 0; cpt < count; cpt++) {
          gras_datadesc_recv_rec(sock, state, refs, sub_type,
                                 r_arch, NULL, 0, ptr, -1,
                                 detect_cycle || sub_type->cycle);

          ptr += elm_size;
        }
      }
      break;
    }

  default:
    xbt_die("Invalid type");
  }

  if (type->recv)
    type->recv(type, state, l_data);

  if (!strcmp(type->name, "string"))
    VERB1("value: '%s'", *(char **) l_data);

}

/**
 * gras_datadesc_recv:
 *
 * Get an instance of the datatype described by @type from the @socket,
 * and store a pointer to it in @dst
 *
 */
void
gras_datadesc_recv(gras_socket_t sock,
                   gras_datadesc_type_t type, int r_arch, void *dst)
{

  xbt_ex_t e;
  static gras_cbps_t state = NULL;      /* callback persistent state */
  static xbt_dict_t refs = NULL;        /* all references already sent */

  if (!state) {
    state = gras_cbps_new();
    refs = xbt_dict_new();
  }

  xbt_assert0(type, "called with NULL type descriptor");
  TRY {
    gras_datadesc_recv_rec(sock, state, refs, type,
                           r_arch, NULL, 0, (char *) dst, -1, type->cycle);
  } CLEANUP {
    xbt_dict_reset(refs);
    gras_cbps_reset(state);
  } CATCH(e) {
    RETHROW;
  }
}
