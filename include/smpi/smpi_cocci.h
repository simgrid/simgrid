/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COCCI_H
#define SMPI_COCCI_H

/* Macros used by coccinelle-generated code */

#define SMPI_VARINIT_GLOBAL(name,type)                       \
type *name = NULL;                                                        \
void __attribute__((weak,constructor)) __preinit_##name(void) { \
   if(!name)                                                    \
      name = (type*)malloc(smpi_global_size() * sizeof(type));  \
}                                                               \
void __attribute__((weak,destructor)) __postfini_##name(void) { \
   free(name);                                                  \
   name = NULL;                                                 \
}

#define SMPI_VARINIT_GLOBAL_AND_SET(name,type,expr)          \
type *name = NULL;                                                        \
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

#endif
