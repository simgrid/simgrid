/* $Id$ */

/* set_usage - A test of normal usage of a set                              */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <assert.h>

#include <gras.h>

GRAS_LOG_NEW_DEFAULT_CATEGORY(test);
GRAS_LOG_EXTERNAL_CATEGORY(set);

typedef struct  {
  /* headers */
  unsigned int ID;
  char        *name;
  unsigned int name_len;

  /* payload */
  char         *data;
}my_elem_t;

static gras_error_t fill(gras_set_t **set);
static gras_error_t debuged_add(gras_set_t *set,const char*key);
static gras_error_t debuged_add_with_data(gras_set_t *set,
					  const char *name,
					  const char *data);
static gras_error_t search_name(gras_set_t *set,const char*key);
static gras_error_t search_id(gras_set_t *head,
			      int id,
			      const char*expected_key);
static gras_error_t traverse(gras_set_t *set);

static void my_elem_free(void *e) {
  my_elem_t *elm=(my_elem_t*)e;

  if (elm) {
    free(elm->name);
    free(elm->data);
    free(elm);
  }
}

static gras_error_t debuged_add_with_data(gras_set_t *set,
					  const char *name,
					  const char *data) {

  gras_error_t  errcode;
  my_elem_t    *elm;

  elm = (my_elem_t*)malloc(sizeof(my_elem_t));
  elm->name=strdup(name);
  elm->name_len=0;

  elm->data=strdup(data);

  printf("   - Add %s ",name);
  if (strcmp(name,data)) {
    printf("(->%s)",data);
  }
  printf("\n");
  errcode=gras_set_add(set,
		       (gras_set_elm_t*)elm,
		       &my_elem_free);
  return errcode;
}

static gras_error_t debuged_add(gras_set_t *set,
				const char *name) {
  return debuged_add_with_data(set, name, name);
}

static gras_error_t fill(gras_set_t **set) {
  gras_error_t errcode;
  printf("\n Fill in the data set\n");

  TRY(gras_set_new(set));
  TRY(debuged_add(*set,"12"));
  TRY(debuged_add(*set,"12a"));
  TRY(debuged_add(*set,"12b"));
  TRY(debuged_add(*set,"123"));
  TRY(debuged_add(*set,"123456"));
  // Child becomes child of what to add
  TRY(debuged_add(*set,"1234"));
  // Need of common ancestor
  TRY(debuged_add(*set,"123457"));

  return no_error;
}

static gras_error_t search_name(gras_set_t *head,const char*key) {
  gras_error_t    errcode;
  my_elem_t      *elm;
  
  errcode=gras_set_get_by_name(head,key,(gras_set_elm_t**)&elm);
  printf("   - Search by name %s. Found %s (under ID %d)\n",
	 key, 
	 elm? elm->data:"(null)",
	 elm? elm->ID:-1);
  if (strcmp(key,elm->name)) {
    printf("    The key (%s) is not the one expected (%s)\n",
	   key,elm->name);
    return mismatch_error;
  }
  if (strcmp(elm->name,elm->data)) {
    printf("    The name (%s) != data (%s)\n",
	   elm->name,elm->data);
    return mismatch_error;
  }
  fflush(stdout);
  return errcode;
}

static gras_error_t search_id(gras_set_t *head,int id,const char*key) {
  gras_error_t    errcode;
  my_elem_t      *elm;
  
  errcode=gras_set_get_by_id(head,id,(gras_set_elm_t**)&elm);
  printf("   - Search by id %d. Found %s (data %s)\n",
	 id, 
	 elm? elm->name:"(null)",
	 elm? elm->data:"(null)");
  if (id != elm->ID) {
    printf("    The found ID (%d) is not the one expected (%d)\n",
	   elm->ID,id);
    return mismatch_error;
  }
  if (strcmp(key,elm->name)) {
    printf("    The key (%s) is not the one expected (%s)\n",
	   elm->name,key);
    return mismatch_error;
  }
  if (strcmp(elm->name,elm->data)) {
    printf("    The name (%s) != data (%s)\n",
	   elm->name,elm->data);
    return mismatch_error;
  }
  fflush(stdout);
  return errcode;
}


static gras_error_t traverse(gras_set_t *set) {
  gras_set_cursor_t *cursor=NULL;
  my_elem_t *elm=NULL;

  gras_set_foreach(set,cursor,elm) {
    gras_assert0(elm,"Dude ! Got a null elm during traversal!");
    printf("   - Id(%d):  %s->%s\n",elm->ID,elm->name,elm->data);
    if (strcmp(elm->name,elm->data)) {
      printf("Key(%s) != value(%s). Abording\n",elm->name,elm->data);
      abort();
    }
  }
  return no_error;
}

int main(int argc,char **argv) {
  gras_error_t errcode;
  gras_set_t *set=NULL;
  my_elem_t *elm;

  gras_init_defaultlog(&argc,argv,"set.thresh=verbose");
   
  printf("\nData set: USAGE test:\n");

  printf(" Traverse the empty set\n");
  TRYFAIL(traverse(set));

  TRYFAIL(fill(&set));
  printf(" Free the data set\n");
  gras_set_free(&set);
  printf(" Free the data set again\n");
  gras_set_free(&set);
  
  TRYFAIL(fill(&set));

  printf(" - Change some values\n");
  printf("   - Change 123 to 'Changed 123'\n");
  TRYFAIL(debuged_add_with_data(set,"123","Changed 123"));
  printf("   - Change 123 back to '123'\n");
  TRYFAIL(debuged_add_with_data(set,"123","123"));
  printf("   - Change 12a to 'Dummy 12a'\n");
  TRYFAIL(debuged_add_with_data(set,"12a","Dummy 12a"));
  printf("   - Change 12a to '12a'\n");
  TRYFAIL(debuged_add_with_data(set,"12a","12a"));

  //  gras_dict_dump(head,(void (*)(void*))&printf);
  printf(" - Traverse the resulting data set\n");
  TRYFAIL(traverse(set));

  printf(" - Retrive values\n");
  TRYFAIL(gras_set_get_by_name(set,"123",(gras_set_elm_t**)&elm));
  assert(elm);
  TRYFAIL(strcmp("123",elm->data));

  TRYEXPECT(gras_set_get_by_name(set,"Can't be found",(gras_set_elm_t**)&elm),
	    mismatch_error);
  TRYEXPECT(gras_set_get_by_name(set,"123 Can't be found",(gras_set_elm_t**)&elm),
	    mismatch_error);
  TRYEXPECT(gras_set_get_by_name(set,"12345678 NOT",(gras_set_elm_t**)&elm),
	    mismatch_error);

  TRYFAIL(search_name(set,"12"));
  TRYFAIL(search_name(set,"12a"));
  TRYFAIL(search_name(set,"12b"));
  TRYFAIL(search_name(set,"123"));
  TRYFAIL(search_name(set,"123456"));
  TRYFAIL(search_name(set,"1234"));
  TRYFAIL(search_name(set,"123457"));

  TRYFAIL(search_id(set,0,"12"));
  TRYFAIL(search_id(set,1,"12a"));
  TRYFAIL(search_id(set,2,"12b"));
  TRYFAIL(search_id(set,3,"123"));
  TRYFAIL(search_id(set,4,"123456"));
  TRYFAIL(search_id(set,5,"1234"));
  TRYFAIL(search_id(set,6,"123457"));

  printf(" - Traverse the resulting data set\n");
  TRYFAIL(traverse(set));

  //  gras_dict_dump(head,(void (*)(void*))&printf);

  printf(" Free the data set (twice)\n");
  gras_set_free(&set);
  gras_set_free(&set);

  printf(" - Traverse the resulting data set\n");
  TRYFAIL(traverse(set));

  gras_exit();
  return 0;
}
