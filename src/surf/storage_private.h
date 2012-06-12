/*
 * storage_private.h
 *
 *  Created on: 2 mars 2012
 *      Author: navarro
 */

#ifndef STORAGE_PRIVATE_H_
#define STORAGE_PRIVATE_H_

typedef struct s_storage_type {
  char *model;
  xbt_dict_t content; /* char * -> s_surf_stat_t */
  char *type_id;
  xbt_dict_t properties;
  unsigned long size;
  unsigned long used_size;
} s_storage_type_t, *storage_type_t;

typedef struct s_mount {
  void *id;
  char *name;
} s_mount_t, *mount_t;

typedef struct surf_stat { /* file status structure */
  s_file_stat_t stat;
  /* possible additionnal fields (e.g., popularity, last access time to know whether the file is in cache, ...) */
} s_surf_stat_t;

typedef struct surf_file {
  char *name;
  surf_stat_t content;
} s_surf_file_t;

typedef struct storage {
  s_surf_resource_t generic_resource;   /*< Structure with generic data. Needed at begin to interate with SURF */
  e_surf_resource_state_t state_current;        /*< STORAGE current state (ON or OFF) */
  lmm_constraint_t constraint;          /* Constraint for maximum bandwidth from connexion */
  lmm_constraint_t constraint_write;    /* Constraint for maximum write bandwidth*/
  lmm_constraint_t constraint_read;     /* Constraint for maximum write bandwidth*/
} s_storage_t, *storage_t;

typedef enum {
  READ, WRITE, STAT, OPEN, CLOSE
} e_surf_action_storage_type_t;

typedef struct surf_action_storage {
  s_surf_action_lmm_t generic_lmm_action;
  e_surf_action_storage_type_t type;
} s_surf_action_storage_t, *surf_action_storage_t;

#endif /* STORAGE_PRIVATE_H_ */
