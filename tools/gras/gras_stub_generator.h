/* 	$Id$	 */

/* gras_stub_generator - creates the main() to use a GRAS program           */

/* Copyright (c) 2003-2007 Martin Quinson, Arnaud Legrand, Malek Cherier.   */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_STUB_GENERATOR_H
#define GRAS_STUB_GENERATOR_H

#define WARN "/***********\n * DO NOT EDIT! THIS FILE HAS BEEN AUTOMATICALLY GENERATED FROM %s BY gras_stub_generator\n ***********/\n"

#include "xbt/dynar.h"
#include "xbt/dict.h"
xbt_dict_t process_function_set;
xbt_dynar_t process_list;
xbt_dict_t machine_set;

typedef struct s_process_t {
  int argc;
  char **argv;
  char *host;
} s_process_t;
char *warning;

void s_process_free(void *process);

/* UNIX files */
void generate_sim(char *project);
void generate_rl(char *project);
void generate_makefile_am(char *project, char *deployment);
void generate_makefile_local(char *project, char *deployment);

/* the gras.h header directory (used only in windows)*/
extern char *__gras_path;

/*
 * Generate a borland simulation poject file.
 * @param name The name of the simulation project
 */
void generate_borland_simulation_project(const char *name);

/*
 * Generate a borland project file for real life.
 * @param name The name of the project to create.
 */
void generate_borland_real_life_project(const char *name);


/*
 * Create the Microsoft visual C++ file project for the simulation.
 */
int generate_simulation_dsp_file(const char *project_name);

/*
 * Create the Microsoft visual C++ real life project file.
 */
int generate_real_live_dsp_file(const char *project_name);


#endif
