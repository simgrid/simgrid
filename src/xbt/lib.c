/* lib - a generic library, variation over dictionary                    */

/* Copyright (c) 2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/lib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_lib, xbt, "A dict with keys of type (name, level)");

xbt_lib_t xbt_lib_new(void)
{
  xbt_lib_t lib  = xbt_new(s_xbt_lib_t, 1);
  lib->dict = xbt_dict_new_homogeneous(xbt_free_f);
  lib->levels = 0;
  lib->free_f = NULL;
  return lib;
}

void xbt_lib_free(xbt_lib_t *plib)
{
  xbt_lib_t lib = *plib;
  if (lib) {
    xbt_dict_cursor_t cursor;
    char *key;
    void **elts;
    xbt_dict_foreach (lib->dict, cursor, key, elts) {
      int i;
      for (i = 0 ; i < lib->levels ; i++)
        if (elts[i] && lib->free_f[i])
          lib->free_f[i](elts[i]);
    }
    xbt_dict_free(&lib->dict);
    xbt_free(lib->free_f);
    xbt_free(lib);
    *plib = NULL;
  }
}

int xbt_lib_add_level(xbt_lib_t lib, void_f_pvoid_t free_f)
{
  XBT_DEBUG("xbt_lib_add_level");
  xbt_assert(xbt_dict_is_empty(lib->dict), "Lib is not empty, cannot add a level");
  lib->free_f = xbt_realloc(lib->free_f, sizeof(void_f_pvoid_t) * (lib->levels + 1));
  lib->free_f[lib->levels] = free_f;
  return lib->levels++;
}

void xbt_lib_set(xbt_lib_t lib, const char *key, int level, void *obj)
{
  XBT_DEBUG("xbt_lib_set key '%s:%d' with object %p", key, level, obj);
  void **elts = xbt_dict_get_or_null(lib->dict, key);
  if (!elts) {
    elts = xbt_new0(void *, lib->levels);
    xbt_dict_set(lib->dict, key, elts, NULL);
  }
  if (elts[level]) {
    XBT_DEBUG("Replace %p by %p element under key '%s:%d'", elts[level], obj, key, level);
    if (lib->free_f[level])
      lib->free_f[level](elts[level]);
  }
  elts[level] = obj;
}

void xbt_lib_unset(xbt_lib_t lib, const char *key, int level, int invoke_callback)
{
  void **elts = xbt_dict_get_or_null(lib->dict, key);
  if (!elts) {
     XBT_WARN("no key %s", key);
     return;
  }

  void *obj = elts[level];
  if (!obj) {
     XBT_WARN("no key %s at level %d", key, level);
     return;
  }

  XBT_DEBUG("Remove %p of key %s at level %d", obj, key, level);
  elts[level] = NULL;

  /* check if there still remains any elements of this key */
  int empty = 1;
  for (int i = 0; i < lib->levels && empty; i++) {
     if (elts[i] != NULL)
       empty = 0;
  }
  if (empty) {
    /* there is no element at any level, so delete the key */
    xbt_dict_remove(lib->dict, key);
  }

  if (invoke_callback && lib->free_f[level])
    lib->free_f[level](obj);
}

void *xbt_lib_get_or_null(xbt_lib_t lib, const char *key, int level)
{
  void **elts = xbt_dict_get_or_null(lib->dict, key);
  return elts ? elts[level] : NULL;
}

xbt_dictelm_t xbt_lib_get_elm_or_null(xbt_lib_t lib, const char *key)
{
  return xbt_dict_get_elm_or_null(lib->dict, key);
}

void *xbt_lib_get_level(xbt_dictelm_t elm, int level){
  void **elts = elm->content;
  return elts ? elts[level] : NULL;
}

void xbt_lib_remove(xbt_lib_t lib, const char *key){
  xbt_dict_remove(lib->dict, key);
}
