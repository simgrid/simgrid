/* Copyright (c) 2011-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COCCI_H
#define SMPI_COCCI_H

#include <xbt/misc.h>

/* Macros used by coccinelle-generated code */

#define SMPI_VARINIT_GLOBAL(name,type)                          \
type *name = NULL;                                              \
void __attribute__((weak,constructor)) __preinit_##name(void) { \
   if(!name)                                                    \
      name = (type*)malloc(smpi_global_size() * sizeof(type));  \
}                                                               \
void __attribute__((weak,destructor)) __postfini_##name(void) { \
   free(name);                                                  \
   name = NULL;                                                 \
}

#define SMPI_VARINIT_GLOBAL_AND_SET(name,type,expr)             \
type *name = NULL;                                              \
void __attribute__((weak,constructor)) __preinit_##name(void) { \
   size_t size = smpi_global_size();                            \
   size_t i;                                                    \
   type value = expr;                                           \
   if(!name) {                                                  \
      name = (type*)malloc(size * sizeof(type));                \
      for(i = 0; i < size; i++) {                               \
         name[i] = value;                                       \
      }                                                         \
   }                                                            \
}                                                               \
void __attribute__((weak,destructor)) __postfini_##name(void) { \
   free(name);                                                  \
   name = NULL;                                                 \
}

#define SMPI_VARGET_GLOBAL(name) name[smpi_process_index()]

/* The following handle local static variables */
/** @brief Make sure that the passed pointer is freed on process exit.
 *
 * This function is rather internal, mainly used for the
 * privatization of global variables through coccinelle.
 */
XBT_PUBLIC(void) smpi_register_static(void* arg, void_f_pvoid_t free_fn);

XBT_PUBLIC(void) smpi_free_static(void);

#define SMPI_VARINIT_STATIC(name,type)                      \
static type *name = NULL;                                   \
if(!name) {                                                 \
   name = (type*)malloc(smpi_global_size() * sizeof(type)); \
   smpi_register_static(name, xbt_free);                    \
}

#define SMPI_VARINIT_STATIC_AND_SET(name,type,expr) \
static type *name = NULL;                           \
if(!name) {                                         \
   size_t size = smpi_global_size();                \
   size_t i;                                        \
   type value = expr;                               \
   name = (type*)malloc(size * sizeof(type));       \
   for(i = 0; i < size; i++) {                      \
      name[i] = value;                              \
   }                                                \
   smpi_register_static(name, xbt_free);            \
}

#define SMPI_VARGET_STATIC(name) name[smpi_process_index()]

#endif
