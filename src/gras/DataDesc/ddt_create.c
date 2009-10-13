/* $Id$ */

/* ddt_new - creation/deletion of datatypes structs (private to this module)*/

/* Copyright (c) 2003-2009 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"           /* min()/max() */
#include "xbt/ex.h"
#include "gras/DataDesc/datadesc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_ddt_create, gras_ddt,
                                "Creating new datadescriptions");

/*** prototypes ***/
static gras_dd_cat_field_t
gras_dd_find_field(gras_datadesc_type_t type, const char *field_name);
/**
 * gras_ddt_freev:
 *
 * gime that memory back, dude. I mean it.
 */
void gras_ddt_freev(void *ddt)
{
  gras_datadesc_type_t type = (gras_datadesc_type_t) ddt;

  if (type) {
    gras_datadesc_free(&type);
  }
}

static gras_datadesc_type_t gras_ddt_new(const char *name)
{
  gras_datadesc_type_t res;

  XBT_IN1("(%s)", name);
  res = xbt_new0(s_gras_datadesc_type_t, 1);

  res->name = (char *) strdup(name);
  res->name_len = strlen(name);
  res->cycle = 0;

  xbt_set_add(gras_datadesc_set_local, (xbt_set_elm_t) res, gras_ddt_freev);
  XBT_OUT;
  return res;
}

/** @brief retrieve an existing message type from its name (or NULL if it does not exist). */
gras_datadesc_type_t gras_datadesc_by_name_or_null(const char *name)
{
  xbt_ex_t e;
  gras_datadesc_type_t res = NULL;

  TRY {
    res = gras_datadesc_by_name(name);
  } CATCH(e) {
    res = NULL;
    xbt_ex_free(e);
  }
  return res;
}

/**
 * Search the given datadesc (or raises an exception if it can't be found)
 */
gras_datadesc_type_t gras_datadesc_by_name(const char *name)
{
  xbt_ex_t e;
  gras_datadesc_type_t res = NULL;
  volatile int found = 0;
  TRY {
    res =
      (gras_datadesc_type_t) xbt_set_get_by_name(gras_datadesc_set_local,
                                                 name);
    found = 1;
  } CATCH(e) {
    if (e.category != not_found_error)
      RETHROW;
    xbt_ex_free(e);
  }
  if (!found)
    THROW1(not_found_error, 0, "No registred datatype of that name: %s",
           name);

  return res;
}

/**
 * Retrieve a type from its code (or NULL if not found)
 */
gras_datadesc_type_t gras_datadesc_by_id(long int code)
{
  xbt_ex_t e;
  gras_datadesc_type_t res = NULL;
  TRY {
    res =
      (gras_datadesc_type_t) xbt_set_get_by_id(gras_datadesc_set_local, code);
  } CATCH(e) {
    if (e.category != not_found_error)
      RETHROW;
    xbt_ex_free(e);
    res = NULL;
  }
  return res;
}

/**
 * Create a new scalar and give a pointer to it
 */
gras_datadesc_type_t
gras_datadesc_scalar(const char *name,
                     gras_ddt_scalar_type_t type,
                     enum e_gras_dd_scalar_encoding encoding)
{

  gras_datadesc_type_t res;
  long int arch;

  XBT_IN;
  res = gras_datadesc_by_name_or_null(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_scalar,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.scalar_data.encoding == encoding,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.scalar_data.type == type,
                "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s", name);
    return res;
  }
  res = gras_ddt_new(name);

  for (arch = 0; arch < gras_arch_count; arch++) {
    res->size[arch] = gras_arches[arch].sizeofs[type];
    res->alignment[arch] = gras_arches[arch].boundaries[type];
    res->aligned_size[arch] =
      ddt_aligned(res->size[arch], res->alignment[arch]);
  }

  res->category_code = e_gras_datadesc_type_cat_scalar;
  res->category.scalar_data.encoding = encoding;
  res->category.scalar_data.type = type;
  XBT_OUT;

  return res;
}


/** Frees one struct or union field */
void gras_dd_cat_field_free(void *f)
{
  gras_dd_cat_field_t field = *(gras_dd_cat_field_t *) f;
  XBT_IN;
  if (field) {
    if (field->name)
      free(field->name);
    free(field);
  }
  XBT_OUT;
}

/** \brief Declare a new structure description */
gras_datadesc_type_t gras_datadesc_struct(const char *name)
{

  gras_datadesc_type_t res;
  long int arch;

  XBT_IN1("(%s)", name);
  res = gras_datadesc_by_name_or_null(name);
  if (res) {
    /* FIXME: Check that field redefinition matches */
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_struct,
                "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s", name);
    return res;
  }
  res = gras_ddt_new(name);

  for (arch = 0; arch < gras_arch_count; arch++) {
    res->size[arch] = 0;
    res->alignment[arch] = 0;
    res->aligned_size[arch] = 0;
  }
  res->category_code = e_gras_datadesc_type_cat_struct;
  res->category.struct_data.fields =
    xbt_dynar_new(sizeof(gras_dd_cat_field_t), gras_dd_cat_field_free);

  XBT_OUT;
  return res;
}

/** \brief Append a new field to a structure description */
void
gras_datadesc_struct_append(gras_datadesc_type_t struct_type,
                            const char *name, gras_datadesc_type_t field_type)
{

  gras_dd_cat_field_t field;
  int arch;

  xbt_assert2(field_type,
              "Cannot add the field '%s' into struct '%s': its type is NULL",
              name, struct_type->name);
  XBT_IN3("(%s %s.%s;)", field_type->name, struct_type->name, name);
  if (struct_type->category.struct_data.closed) {
    VERB1
      ("Ignoring request to add field to struct %s (closed. Redefinition?)",
       struct_type->name);
    return;
  }

  xbt_assert1(field_type->size[GRAS_THISARCH] >= 0,
              "Cannot add a dynamically sized field in structure %s",
              struct_type->name);

  field = xbt_new(s_gras_dd_cat_field_t, 1);
  field->name = (char *) strdup(name);

  DEBUG0("----------------");
  DEBUG3("PRE s={size=%ld,align=%ld,asize=%ld}",
         struct_type->size[GRAS_THISARCH],
         struct_type->alignment[GRAS_THISARCH],
         struct_type->aligned_size[GRAS_THISARCH]);


  for (arch = 0; arch < gras_arch_count; arch++) {
    field->offset[arch] = ddt_aligned(struct_type->size[arch],
                                      field_type->alignment[arch]);

    struct_type->size[arch] = field->offset[arch] + field_type->size[arch];
    struct_type->alignment[arch] = max(struct_type->alignment[arch],
                                       field_type->alignment[arch]);
    struct_type->aligned_size[arch] = ddt_aligned(struct_type->size[arch],
                                                  struct_type->alignment
                                                  [arch]);
  }
  field->type = field_type;
  field->send = NULL;
  field->recv = NULL;

  xbt_dynar_push(struct_type->category.struct_data.fields, &field);

  DEBUG3("Push a %s into %s at offset %ld.",
         field_type->name, struct_type->name, field->offset[GRAS_THISARCH]);
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

/** \brief Close a structure description
 *
 * No new field can be added afterward, and it is mandatory to close the structure before using it.
 */
void gras_datadesc_struct_close(gras_datadesc_type_t struct_type)
{
  int arch;
  XBT_IN;
  struct_type->category.struct_data.closed = 1;
  for (arch = 0; arch < gras_arch_count; arch++) {
    struct_type->size[arch] = struct_type->aligned_size[arch];
  }
  DEBUG4("structure %s closed. size=%ld,align=%ld,asize=%ld",
         struct_type->name,
         struct_type->size[GRAS_THISARCH],
         struct_type->alignment[GRAS_THISARCH],
         struct_type->aligned_size[GRAS_THISARCH]);
}

/**
 * gras_datadesc_cycle_set:
 *
 * Tell GRAS that the pointers of the type described by ddt may present
 * some loop, and that the cycle detection mechanism is needed.
 *
 * Note that setting this option when not needed have a rather bad effect
 * on the performance (several times slower on big data).
 */
void gras_datadesc_cycle_set(gras_datadesc_type_t ddt)
{
  ddt->cycle = 1;
}

/**
 * gras_datadesc_cycle_unset:
 *
 * Tell GRAS that the pointers of the type described by ddt do not present
 * any loop and that cycle detection mechanism are not needed.
 * (default)
 */
void gras_datadesc_cycle_unset(gras_datadesc_type_t ddt)
{
  ddt->cycle = 0;
}

/** \brief Declare a new union description */
gras_datadesc_type_t
gras_datadesc_union(const char *name, gras_datadesc_type_cb_int_t selector)
{

  gras_datadesc_type_t res;
  int arch;

  XBT_IN1("(%s)", name);
  xbt_assert0(selector,
              "Attempt to creat an union without field_count function");

  res = gras_datadesc_by_name_or_null(name);
  if (res) {
    /* FIXME: Check that field redefinition matches */
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_union,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.union_data.selector == selector,
                "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s", name);
    return res;
  }

  res = gras_ddt_new(name);

  for (arch = 0; arch < gras_arch_count; arch++) {
    res->size[arch] = 0;
    res->alignment[arch] = 0;
    res->aligned_size[arch] = 0;
  }

  res->category_code = e_gras_datadesc_type_cat_union;
  res->category.union_data.fields =
    xbt_dynar_new(sizeof(gras_dd_cat_field_t *), gras_dd_cat_field_free);
  res->category.union_data.selector = selector;

  return res;
}

/** \brief Append a new field to an union description */
void gras_datadesc_union_append(gras_datadesc_type_t union_type,
                                const char *name,
                                gras_datadesc_type_t field_type)
{

  gras_dd_cat_field_t field;
  int arch;

  XBT_IN3("(%s %s.%s;)", field_type->name, union_type->name, name);
  xbt_assert1(field_type->size[GRAS_THISARCH] >= 0,
              "Cannot add a dynamically sized field in union %s",
              union_type->name);

  if (union_type->category.union_data.closed) {
    VERB1("Ignoring request to add field to union %s (closed)",
          union_type->name);
    return;
  }

  field = xbt_new0(s_gras_dd_cat_field_t, 1);

  field->name = (char *) strdup(name);
  field->type = field_type;
  /* All offset are left to 0 in an union */

  xbt_dynar_push(union_type->category.union_data.fields, &field);

  for (arch = 0; arch < gras_arch_count; arch++) {
    union_type->size[arch] = max(union_type->size[arch],
                                 field_type->size[arch]);
    union_type->alignment[arch] = max(union_type->alignment[arch],
                                      field_type->alignment[arch]);
    union_type->aligned_size[arch] = ddt_aligned(union_type->size[arch],
                                                 union_type->alignment[arch]);
  }
}


/** \brief Close an union description
 *
 * No new field can be added afterward, and it is mandatory to close the union before using it.
 */
void gras_datadesc_union_close(gras_datadesc_type_t union_type)
{
  union_type->category.union_data.closed = 1;
}

/** \brief Copy a type under another name
 *
 * This may reveal useful to circumvent parsing macro limitations
 */
gras_datadesc_type_t
gras_datadesc_copy(const char *name, gras_datadesc_type_t copied)
{

  gras_datadesc_type_t res = gras_ddt_new(name);
  char *name_cpy = res->name;

  memcpy(res, copied, sizeof(s_gras_datadesc_type_t));
  res->name = name_cpy;
  return res;
}

/** \brief Declare a new type being a reference to the one passed in arg */
gras_datadesc_type_t
gras_datadesc_ref(const char *name, gras_datadesc_type_t referenced_type)
{

  gras_datadesc_type_t res;
  gras_datadesc_type_t pointer_type = gras_datadesc_by_name("data pointer");
  int arch;

  XBT_IN1("(%s)", name);
  res = gras_datadesc_by_name_or_null(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_ref,
                "Redefinition of %s does not match", name);
    xbt_assert1(res->category.ref_data.type == referenced_type,
                "Redefinition of %s does not match", name);
    xbt_assert1(res->category.ref_data.selector == NULL,
                "Redefinition of %s does not match", name);
    VERB1("Discarding redefinition of %s", name);
    return res;
  }

  res = gras_ddt_new(name);

  xbt_assert0(pointer_type, "Cannot get the description of data pointer");

  for (arch = 0; arch < gras_arch_count; arch++) {
    res->size[arch] = pointer_type->size[arch];
    res->alignment[arch] = pointer_type->alignment[arch];
    res->aligned_size[arch] = pointer_type->aligned_size[arch];
  }

  res->category_code = e_gras_datadesc_type_cat_ref;
  res->category.ref_data.type = referenced_type;
  res->category.ref_data.selector = NULL;

  return res;
}

/** \brief Declare a new type being a generic reference.
 *
 * The callback passed in argument is to be used to select which type is currently used.
 * So, when GRAS wants to send a generic reference, it passes the current data to the selector
 * callback and expects it to return the type description to use.
 */
gras_datadesc_type_t
gras_datadesc_ref_generic(const char *name, gras_datadesc_selector_t selector)
{

  gras_datadesc_type_t res;
  gras_datadesc_type_t pointer_type = gras_datadesc_by_name("data pointer");
  int arch;

  XBT_IN1("(%s)", name);
  res = gras_datadesc_by_name_or_null(name);

  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_ref,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.ref_data.type == NULL,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.ref_data.selector == selector,
                "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s", name);
    return res;
  }
  res = gras_ddt_new(name);

  xbt_assert0(pointer_type, "Cannot get the description of data pointer");

  for (arch = 0; arch < gras_arch_count; arch++) {
    res->size[arch] = pointer_type->size[arch];
    res->alignment[arch] = pointer_type->alignment[arch];
    res->aligned_size[arch] = pointer_type->aligned_size[arch];
  }

  res->category_code = e_gras_datadesc_type_cat_ref;

  res->category.ref_data.type = NULL;
  res->category.ref_data.selector = selector;

  return res;
}

/** \brief Declare a new type being an array of fixed size and content */
gras_datadesc_type_t
gras_datadesc_array_fixed(const char *name,
                          gras_datadesc_type_t element_type,
                          long int fixed_size)
{

  gras_datadesc_type_t res;
  int arch;

  XBT_IN1("(%s)", name);
  res = gras_datadesc_by_name_or_null(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_array,
                "Redefinition of type %s does not match", name);

    if (res->category.array_data.type != element_type) {
      ERROR1("Redefinition of type %s does not match: array elements differ",
             name);
      gras_datadesc_type_dump(res->category.array_data.type);
      gras_datadesc_type_dump(element_type);
    }

    xbt_assert1(res->category.array_data.fixed_size == fixed_size,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.dynamic_size == NULL,
                "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s", name);

    return res;
  }
  res = gras_ddt_new(name);

  xbt_assert1(fixed_size >= 0, "'%s' is a array of negative fixed size",
              name);
  for (arch = 0; arch < gras_arch_count; arch++) {
    res->size[arch] = fixed_size * element_type->aligned_size[arch];
    res->alignment[arch] = element_type->alignment[arch];
    res->aligned_size[arch] = res->size[arch];
  }

  res->category_code = e_gras_datadesc_type_cat_array;

  res->category.array_data.type = element_type;
  res->category.array_data.fixed_size = fixed_size;
  res->category.array_data.dynamic_size = NULL;

  return res;
}

/** \brief Declare a new type being an array of fixed size, but accepting several content types. */
gras_datadesc_type_t gras_datadesc_array_dyn(const char *name,
                                             gras_datadesc_type_t
                                             element_type,
                                             gras_datadesc_type_cb_int_t
                                             dynamic_size)
{

  gras_datadesc_type_t res;
  int arch;

  XBT_IN1("(%s)", name);
  xbt_assert1(dynamic_size,
              "'%s' is a dynamic array without size discriminant", name);

  res = gras_datadesc_by_name_or_null(name);
  if (res) {
    xbt_assert1(res->category_code == e_gras_datadesc_type_cat_array,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.type == element_type,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.fixed_size == -1,
                "Redefinition of type %s does not match", name);
    xbt_assert1(res->category.array_data.dynamic_size == dynamic_size,
                "Redefinition of type %s does not match", name);
    VERB1("Discarding redefinition of %s", name);

    return res;
  }

  res = gras_ddt_new(name);

  for (arch = 0; arch < gras_arch_count; arch++) {
    res->size[arch] = 0;        /* make sure it indicates "dynamic" */
    res->alignment[arch] = element_type->alignment[arch];
    res->aligned_size[arch] = 0;        /*FIXME: That was so in GS, but looks stupid */
  }

  res->category_code = e_gras_datadesc_type_cat_array;

  res->category.array_data.type = element_type;
  res->category.array_data.fixed_size = -1;
  res->category.array_data.dynamic_size = dynamic_size;

  return res;
}

/** \brief Declare a new type being an array which size can be found with \ref gras_cbps_i_pop
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
gras_datadesc_ref_pop_arr(gras_datadesc_type_t element_type)
{

  gras_datadesc_type_t res,ddt2;
  char *name = (char *) xbt_malloc(strlen(element_type->name) + 4);

  sprintf(name, "%s[]", element_type->name);
  /* Make sure we are not trying to redefine a ddt with the same name */
  ddt2 = gras_datadesc_by_name_or_null(name);
  int cpt=0;
  while (ddt2) {
    free(name);
    name=bprintf("%s[]_%d",element_type->name,cpt++);
    ddt2=gras_datadesc_by_name_or_null(name);
  }

  res = gras_datadesc_array_dyn(name, element_type, gras_datadesc_cb_pop);

  sprintf(name, "%s[]*", element_type->name);
  cpt=0;
  ddt2 = gras_datadesc_by_name_or_null(name);
  while (ddt2) {
    free(name);
    name=bprintf("%s[]*_%d",element_type->name,cpt++);
    ddt2=gras_datadesc_by_name_or_null(name);
  }

  res = gras_datadesc_ref(name, res);

  free(name);

  return res;
}

/*
 *##
 *## Constructor of container datatypes
 *##
 */

static void gras_datadesc_dynar_cb(gras_datadesc_type_t typedesc,
                                   gras_cbps_t vars, void *data)
{
  gras_datadesc_type_t subtype;
  xbt_dynar_t dynar = (xbt_dynar_t) data;

  memcpy(&dynar->free_f, &typedesc->extra, sizeof(dynar->free_f));

  /* search for the elemsize in what we have. If elements are "int", typedesc got is "int[]*" */
  subtype = gras_dd_find_field(typedesc, "data")->type;

  /* this is now a ref to array of what we're looking for */
  subtype = subtype->category.ref_data.type;
  subtype = subtype->category.array_data.type;

  DEBUG1("subtype is %s", subtype->name);

  dynar->elmsize = subtype->size[GRAS_THISARCH];
  dynar->size = dynar->used;
  dynar->mutex = NULL;
}

/** \brief Declare a new type being a dynar in which each elements are of the given type
 *
 *  The type gets registered under the name "dynar(%s)_s", where %s is the name of the subtype.
 *  For example, a dynar of doubles will be called "dynar(double)_s" and a dynar of dynar of
 *  strings will be called "dynar(dynar(string)_s)_s".
 *
 *  \param elm_t: the datadesc of the elements
 *  \param free_func: the function to use to free the elements when the dynar gets freed
 */
gras_datadesc_type_t
gras_datadesc_dynar(gras_datadesc_type_t elm_t, void_f_pvoid_t free_func)
{

  char *buffname;
  gras_datadesc_type_t res;

  asprintf(&buffname, "s_xbt_dynar_of_%s", elm_t->name);

  res = gras_datadesc_struct(buffname);

  gras_datadesc_struct_append(res, "size",
                              gras_datadesc_by_name("unsigned long int"));

  gras_datadesc_struct_append(res, "used",
                              gras_datadesc_by_name("unsigned long int"));

  gras_datadesc_struct_append(res, "elmsize",
                              gras_datadesc_by_name("unsigned long int"));

  gras_datadesc_struct_append(res, "data", gras_datadesc_ref_pop_arr(elm_t));

  gras_datadesc_struct_append(res, "free_f",
                              gras_datadesc_by_name("function pointer"));
  memcpy(res->extra, &free_func, sizeof(free_func));

  gras_datadesc_struct_append(res, "mutex",
                              gras_datadesc_by_name("data pointer"));

  gras_datadesc_struct_close(res);

  gras_datadesc_cb_field_push(res, "used");
  gras_datadesc_cb_recv(res, &gras_datadesc_dynar_cb);

  /* build a ref to it */
  free(buffname);
  asprintf(&buffname, "xbt_dynar_of_%s", elm_t->name);
  res = gras_datadesc_ref(buffname, res);
  free(buffname);
  return res;
}

#include "xbt/matrix.h"
static void gras_datadesc_matrix_cb(gras_datadesc_type_t typedesc,
                                    gras_cbps_t vars, void *data)
{
  gras_datadesc_type_t subtype;
  xbt_matrix_t matrix = (xbt_matrix_t) data;

  memcpy(&matrix->free_f, &typedesc->extra, sizeof(matrix->free_f));

  /* search for the elemsize in what we have. If elements are "int", typedesc got is "int[]*" */
  subtype = gras_dd_find_field(typedesc, "data")->type;

  /* this is now a ref to array of what we're looking for */
  subtype = subtype->category.ref_data.type;
  subtype = subtype->category.array_data.type;

  DEBUG1("subtype is %s", subtype->name);

  matrix->elmsize = subtype->size[GRAS_THISARCH];
}

gras_datadesc_type_t
gras_datadesc_matrix(gras_datadesc_type_t elm_t, void_f_pvoid_t const free_f)
{
  char *buffname;
  gras_datadesc_type_t res;

  asprintf(&buffname, "s_xbt_matrix_t(%s)", elm_t->name);
  res = gras_datadesc_struct(buffname);

  gras_datadesc_struct_append(res, "lines",
                              gras_datadesc_by_name("unsigned int"));
  gras_datadesc_struct_append(res, "rows",
                              gras_datadesc_by_name("unsigned int"));

  gras_datadesc_struct_append(res, "elmsize",
                              gras_datadesc_by_name("unsigned long int"));

  gras_datadesc_struct_append(res, "data", gras_datadesc_ref_pop_arr(elm_t));
  gras_datadesc_struct_append(res, "free_f",
                              gras_datadesc_by_name("function pointer"));
  gras_datadesc_struct_close(res);

  gras_datadesc_cb_field_push(res, "lines");
  gras_datadesc_cb_field_push_multiplier(res, "rows");

  gras_datadesc_cb_recv(res, &gras_datadesc_matrix_cb);
  memcpy(res->extra, &free_f, sizeof(free_f));

  /* build a ref to it */
  free(buffname);
  asprintf(&buffname, "xbt_matrix_t(%s)", elm_t->name);
  res = gras_datadesc_ref(buffname, res);
  free(buffname);
  return res;
}

gras_datadesc_type_t
gras_datadesc_import_nws(const char *name,
                         const DataDescriptor * desc, unsigned long howmany)
{
  THROW_UNIMPLEMENTED;
}

/**
 * (useful to push the sizes of the upcoming arrays, for example)
 */
void gras_datadesc_cb_send(gras_datadesc_type_t type,
                           gras_datadesc_type_cb_void_t send)
{
  type->send = send;
}

/**
 * (useful to put the function pointers to the rigth value, for example)
 */
void gras_datadesc_cb_recv(gras_datadesc_type_t type,
                           gras_datadesc_type_cb_void_t recv)
{
  type->recv = recv;
}

/*
 * gras_dd_find_field:
 *
 * Returns the type descriptor of the given field. Abort on error.
 */
static gras_dd_cat_field_t
gras_dd_find_field(gras_datadesc_type_t type, const char *field_name)
{
  xbt_dynar_t field_array;

  gras_dd_cat_field_t field = NULL;
  unsigned int field_num;

  if (type->category_code == e_gras_datadesc_type_cat_union) {
    field_array = type->category.union_data.fields;
  } else if (type->category_code == e_gras_datadesc_type_cat_struct) {
    field_array = type->category.struct_data.fields;
  } else {
    ERROR2("%s (%p) is not a struct nor an union. There is no field.",
           type->name, (void *) type);
    xbt_abort();
  }
  xbt_dynar_foreach(field_array, field_num, field) {
    if (!strcmp(field_name, field->name)) {
      return field;
    }
  }
  ERROR2("No field named '%s' in '%s'", field_name, type->name);
  xbt_abort();

}

/**
 * The given datadesc must be a struct or union (abort if not).
 * (useful to push the sizes of the upcoming arrays, for example)
 */
void gras_datadesc_cb_field_send(gras_datadesc_type_t type,
                                 const char *field_name,
                                 gras_datadesc_type_cb_void_t send)
{

  gras_dd_cat_field_t field = gras_dd_find_field(type, field_name);
  field->send = send;
}


/**
 * The value, which must be an int, unsigned int, long int or unsigned long int
 * is pushed to the stacks of sizes and can then be retrieved with
 * \ref gras_datadesc_ref_pop_arr or directly with \ref gras_cbps_i_pop.
 */
void gras_datadesc_cb_field_push(gras_datadesc_type_t type,
                                 const char *field_name)
{

  gras_dd_cat_field_t field = gras_dd_find_field(type, field_name);
  gras_datadesc_type_t sub_type = field->type;

  DEBUG3("add a PUSHy cb to '%s' field (type '%s') of '%s'",
         field_name, sub_type->name, type->name);
  if (!strcmp("int", sub_type->name)) {
    field->send = gras_datadesc_cb_push_int;
  } else if (!strcmp("unsigned int", sub_type->name)) {
    field->send = gras_datadesc_cb_push_uint;
  } else if (!strcmp("long int", sub_type->name)) {
    field->send = gras_datadesc_cb_push_lint;
  } else if (!strcmp("unsigned long int", sub_type->name)) {
    field->send = gras_datadesc_cb_push_ulint;
  } else {
    ERROR1
      ("Field %s is not an int, unsigned int, long int neither unsigned long int",
       sub_type->name);
    xbt_abort();
  }
}

/**
 * Any previously pushed value is poped and the field value is multiplied to
 * it. The result is then pushed back into the stack of sizes. It can then be
 * retrieved with \ref gras_datadesc_ref_pop_arr or directly with \ref
 * gras_cbps_i_pop.
 *
 * The field must be an int, unsigned int, long int or unsigned long int.
 */
void gras_datadesc_cb_field_push_multiplier(gras_datadesc_type_t type,
                                            const char *field_name)
{

  gras_dd_cat_field_t field = gras_dd_find_field(type, field_name);
  gras_datadesc_type_t sub_type = field->type;

  DEBUG3("add a MPUSHy cb to '%s' field (type '%s') of '%s'",
         field_name, sub_type->name, type->name);
  if (!strcmp("int", sub_type->name)) {
    field->send = gras_datadesc_cb_push_int_mult;
  } else if (!strcmp("unsigned int", sub_type->name)) {
    field->send = gras_datadesc_cb_push_uint_mult;
  } else if (!strcmp("long int", sub_type->name)) {
    field->send = gras_datadesc_cb_push_lint_mult;
  } else if (!strcmp("unsigned long int", sub_type->name)) {
    field->send = gras_datadesc_cb_push_ulint_mult;
  } else {
    ERROR1
      ("Field %s is not an int, unsigned int, long int neither unsigned long int",
       sub_type->name);
    xbt_abort();
  }
}

/**
 * The given datadesc must be a struct or union (abort if not).
 * (useful to put the function pointers to the right value, for example)
 */
void gras_datadesc_cb_field_recv(gras_datadesc_type_t type,
                                 const char *field_name,
                                 gras_datadesc_type_cb_void_t recv)
{

  gras_dd_cat_field_t field = gras_dd_find_field(type, field_name);
  field->recv = recv;
}

/*
 * Free a datadesc. Should only be called at xbt_exit.
 */
void gras_datadesc_free(gras_datadesc_type_t * type)
{

  DEBUG1("Let's free ddt %s", (*type)->name);

  switch ((*type)->category_code) {
  case e_gras_datadesc_type_cat_scalar:
  case e_gras_datadesc_type_cat_ref:
  case e_gras_datadesc_type_cat_array:
    /* nothing to free in there */
    break;

  case e_gras_datadesc_type_cat_struct:
    xbt_dynar_free(&((*type)->category.struct_data.fields));
    break;

  case e_gras_datadesc_type_cat_union:
    xbt_dynar_free(&((*type)->category.union_data.fields));
    break;

  default:
    /* datadesc was invalid. Killing it is like euthanasy, I guess */
    break;
  }
  free((*type)->name);
  free(*type);
  type = NULL;
}

/**
 * gras_datadesc_type_cmp:
 *
 * Compares two datadesc types with the same semantic than strcmp.
 *
 * This comparison does not take the set headers into account (name and ID),
 * but only the payload (actual type description).
 */
int gras_datadesc_type_cmp(const gras_datadesc_type_t d1,
                           const gras_datadesc_type_t d2)
{
  int ret;
  unsigned int cpt;
  gras_dd_cat_field_t field1, field2;
  gras_datadesc_type_t field_desc_1, field_desc_2;

  if (d1 == d2)
    return 0;                   /* easy optimization */

  if (!d1 && d2) {
    DEBUG0("ddt_cmp: !d1 && d2 => 1");
    return 1;
  }
  if (!d1 && !d2) {
    DEBUG0("ddt_cmp: !d1 && !d2 => 0");
    return 0;
  }
  if (d1 && !d2) {
    DEBUG0("ddt_cmp: d1 && !d2 => -1");
    return -1;
  }

  for (cpt = 0; cpt < gras_arch_count; cpt++) {
    if (d1->size[cpt] != d2->size[cpt]) {
      DEBUG5("ddt_cmp: %s->size=%ld  !=  %s->size=%ld (on %s)",
             d1->name, d1->size[cpt], d2->name, d2->size[cpt],
             gras_arches[cpt].name);
      return d1->size[cpt] > d2->size[cpt] ? 1 : -1;
    }

    if (d1->alignment[cpt] != d2->alignment[cpt]) {
      DEBUG5("ddt_cmp: %s->alignment=%ld  !=  %s->alignment=%ld (on %s)",
             d1->name, d1->alignment[cpt], d2->name, d2->alignment[cpt],
             gras_arches[cpt].name);
      return d1->alignment[cpt] > d2->alignment[cpt] ? 1 : -1;
    }

    if (d1->aligned_size[cpt] != d2->aligned_size[cpt]) {
      DEBUG5
        ("ddt_cmp: %s->aligned_size=%ld  !=  %s->aligned_size=%ld (on %s)",
         d1->name, d1->aligned_size[cpt], d2->name, d2->aligned_size[cpt],
         gras_arches[cpt].name);
      return d1->aligned_size[cpt] > d2->aligned_size[cpt] ? 1 : -1;
    }
  }

  if (d1->category_code != d2->category_code) {
    DEBUG4("ddt_cmp: %s->cat=%s  !=  %s->cat=%s",
           d1->name, gras_datadesc_cat_names[d1->category_code],
           d2->name, gras_datadesc_cat_names[d2->category_code]);
    return d1->category_code > d2->category_code ? 1 : -1;
  }

  if (d1->send != d2->send) {
    DEBUG4("ddt_cmp: %s->send=%p  !=  %s->send=%p",
           d1->name, (void *) d1->send, d2->name, (void *) d2->send);
    return 1;                   /* ISO C forbids ordered comparisons of pointers to functions */
  }

  if (d1->recv != d2->recv) {
    DEBUG4("ddt_cmp: %s->recv=%p  !=  %s->recv=%p",
           d1->name, (void *) d1->recv, d2->name, (void *) d2->recv);
    return 1;                   /* ISO C forbids ordered comparisons of pointers to functions */
  }

  switch (d1->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    if (d1->category.scalar_data.encoding !=
        d2->category.scalar_data.encoding)
      return d1->category.scalar_data.encoding >
        d2->category.scalar_data.encoding ? 1 : -1;
    break;

  case e_gras_datadesc_type_cat_struct:
    if (xbt_dynar_length(d1->category.struct_data.fields) !=
        xbt_dynar_length(d2->category.struct_data.fields)) {
      DEBUG4("ddt_cmp: %s (having %lu fields) !=  %s (having %lu fields)",
             d1->name, xbt_dynar_length(d1->category.struct_data.fields),
             d2->name, xbt_dynar_length(d2->category.struct_data.fields));

      return xbt_dynar_length(d1->category.struct_data.fields) >
        xbt_dynar_length(d2->category.struct_data.fields) ? 1 : -1;
    }
    xbt_dynar_foreach(d1->category.struct_data.fields, cpt, field1) {

      field2 =
        xbt_dynar_get_as(d2->category.struct_data.fields, cpt,
                         gras_dd_cat_field_t);
      field_desc_1 = field1->type;
      field_desc_2 = field2->type;
      ret = gras_datadesc_type_cmp(field_desc_1, field_desc_2);
      if (ret) {
        DEBUG6("%s->field[%d]=%s != %s->field[%d]=%s",
               d1->name, cpt, field1->name, d2->name, cpt, field2->name);
        return ret;
      }

    }
    break;

  case e_gras_datadesc_type_cat_union:
    if (d1->category.union_data.selector != d2->category.union_data.selector)
      return 1;                 /* ISO C forbids ordered comparisons of pointers to functions */

    if (xbt_dynar_length(d1->category.union_data.fields) !=
        xbt_dynar_length(d2->category.union_data.fields))
      return xbt_dynar_length(d1->category.union_data.fields) >
        xbt_dynar_length(d2->category.union_data.fields) ? 1 : -1;

    xbt_dynar_foreach(d1->category.union_data.fields, cpt, field1) {

      field2 =
        xbt_dynar_get_as(d2->category.union_data.fields, cpt,
                         gras_dd_cat_field_t);
      field_desc_1 = field1->type;
      field_desc_2 = field2->type;
      ret = gras_datadesc_type_cmp(field_desc_1, field_desc_2);
      if (ret)
        return ret;

    }
    break;


  case e_gras_datadesc_type_cat_ref:
    if (d1->category.ref_data.selector != d2->category.ref_data.selector)
      return 1;                 /* ISO C forbids ordered comparisons of pointers to functions */

    if (d1->category.ref_data.type != d2->category.ref_data.type)
      return d1->category.ref_data.type > d2->category.ref_data.type ? 1 : -1;
    break;

  case e_gras_datadesc_type_cat_array:
    if (d1->category.array_data.type != d2->category.array_data.type)
      return d1->category.array_data.type >
        d2->category.array_data.type ? 1 : -1;

    if (d1->category.array_data.fixed_size !=
        d2->category.array_data.fixed_size)
      return d1->category.array_data.fixed_size >
        d2->category.array_data.fixed_size ? 1 : -1;

    if (d1->category.array_data.dynamic_size !=
        d2->category.array_data.dynamic_size)
      return 1;                 /* ISO C forbids ordered comparisons of pointers to functions */

    break;

  default:
    /* two stupidly created ddt are equally stupid ;) */
    break;
  }
  return 0;

}
