/* Copyright (c) 2009, 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef STORAGE_PRIVATE_H_
#define STORAGE_PRIVATE_H_

typedef struct s_storage_type {
  char *model;
  char *content;
  char *type_id;
  xbt_dict_t properties;
  unsigned long size;
} s_storage_type_t, *storage_type_t;

typedef struct s_mount {
  void *id;
  char *name;
} s_mount_t, *mount_t;

typedef struct surf_file {
  char *name;
  const char* storage;
  size_t size;
} s_surf_file_t;

typedef struct storage {
  s_surf_resource_t generic_resource;   /*< Structure with generic data. Needed at begin to interact with SURF */
  e_surf_resource_state_t state_current;        /*< STORAGE current state (ON or OFF) */
  lmm_constraint_t constraint;          /* Constraint for maximum bandwidth from connection */
  lmm_constraint_t constraint_write;    /* Constraint for maximum write bandwidth*/
  lmm_constraint_t constraint_read;     /* Constraint for maximum write bandwidth*/
  xbt_dict_t content; /* char * -> s_surf_file_t */
  unsigned long size;
  unsigned long used_size;
  xbt_dynar_t write_actions;
} s_storage_t, *storage_t;

typedef enum {
  READ=0, WRITE, STAT, OPEN, CLOSE, UNLINK, LS, GET_SIZE
} e_surf_action_storage_type_t;

typedef struct surf_action_storage {
  s_surf_action_lmm_t generic_lmm_action;
  e_surf_action_storage_type_t type;
  void *storage;
} s_surf_action_storage_t, *surf_action_storage_t;

#endif /* STORAGE_PRIVATE_H_ */
