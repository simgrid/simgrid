/* Copyright (c) 2009 Da SimGrid Team.  All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simdag/simdag.h"
#include "xbt/misc.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_daxparse, sd,"Parsing DAX files");

#undef CLEANUP
#include "dax_dtd.h"
#include "dax_dtd.c"

static YY_BUFFER_STATE input_buffer;

static xbt_dynar_t result;

static void SD_task_free(void*t){
  SD_task_destroy(t);
}

xbt_dynar_t SD_daxload(const char*filename) {
  FILE* in_file = fopen(filename,"r");
  xbt_assert1(in_file, "Unable to open \"%s\"\n", filename);
  input_buffer =
    dax__create_buffer(in_file, 10);
  dax__switch_to_buffer(input_buffer);
  dax_lineno = 1;

  result = xbt_dynar_new(sizeof(SD_task_t),SD_task_free);
  xbt_assert1(dax_lex(),"Parse error in %s",filename);
  dax__delete_buffer(input_buffer);
  fclose(in_file);
  return result;
}

void STag_dax__adag(void) {

}
void STag_dax__job(void) {

}
void STag_dax__child(void) {

}
void STag_dax__parent(void) {

}
void ETag_dax__adag(void) {

}
void ETag_dax__job(void) {

}
void ETag_dax__child(void) {

}
void ETag_dax__parent(void) {

}
