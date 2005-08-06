/* $Id$ */

/* set_usage - A test of normal usage of a set                              */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>

#include "gras.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");
XBT_LOG_EXTERNAL_CATEGORY(set);

typedef struct  {
  /* headers */
  unsigned int ID;
  char        *name;
  unsigned int name_len;

  /* payload */
  char         *data;
} s_my_elem_t,*my_elem_t;

static void fill(xbt_set_t *set);
static void debuged_add(xbt_set_t set,const char*key);
static void debuged_add_with_data(xbt_set_t  set,
				  const char *name,
				  const char *data);
static void search_name(xbt_set_t set,const char*key);
static void search_id(xbt_set_t head,int id,
		      const char*expected_key);
static void traverse(xbt_set_t set);

static void my_elem_free(void *e) {
  my_elem_t elm=(my_elem_t)e;

  if (elm) {
    free(elm->name);
    free(elm->data);
    free(elm);
  }
}

static void debuged_add_with_data(xbt_set_t  set,
				  const char *name,
				  const char *data) {

  my_elem_t    elm;

  elm = xbt_new(s_my_elem_t,1);
  elm->name=xbt_strdup(name);
  elm->name_len=0;

  elm->data=xbt_strdup(data);

  printf("   - Add %s ",name);
  if (strcmp(name,data)) {
    printf("(->%s)",data);
  }
  printf("\n");
  xbt_set_add(set, (xbt_set_elm_t)elm,
	       &my_elem_free);
}

static void debuged_add(xbt_set_t  set,
			const char *name) {
  debuged_add_with_data(set, name, name);
}

static void fill(xbt_set_t *set) {
  printf("\n Fill in the data set\n");

  *set=xbt_set_new();
  debuged_add(*set,"12");
  debuged_add(*set,"12a");
  debuged_add(*set,"12b");
  debuged_add(*set,"123");
  debuged_add(*set,"123456");
  /* Child becomes child of what to add */
  debuged_add(*set,"1234");
  /* Need of common ancestor */
  debuged_add(*set,"123457");
}

static void search_name(xbt_set_t head,const char*key) {
  my_elem_t elm = (my_elem_t)xbt_set_get_by_name(head,key);

  printf("   - Search by name %s. Found %s (under ID %d)\n",
	 key, 
	 elm? elm->data:"(null)",
	 elm? elm->ID:-1);
  if (strcmp(key,elm->name))
    THROW2(mismatch_error,0,"The key (%s) is not the one expected (%s)",
	   key,elm->name);
  if (strcmp(elm->name,elm->data))
    THROW2(mismatch_error,0,"The name (%s) != data (%s)",
	   key,elm->name);
  fflush(stdout);
}

static void search_id(xbt_set_t head,int id,const char*key) {
  my_elem_t elm = (my_elem_t) xbt_set_get_by_id(head,id);

  printf("   - Search by id %d. Found %s (data %s)\n",
	 id, 
	 elm? elm->name:"(null)",
	 elm? elm->data:"(null)");
  if (id != elm->ID)
    THROW2(mismatch_error,0,"The found ID (%d) is not the one expected (%d)",
	   elm->ID,id);
  if (strcmp(key,elm->name))
    THROW2(mismatch_error,0,"The key (%s) is not the one expected (%s)",
	   elm->name,key);
  if (strcmp(elm->name,elm->data))
    THROW2(mismatch_error,0,"The name (%s) != data (%s)",
	   elm->name,elm->data);
  fflush(stdout);
}


static void traverse(xbt_set_t set) {
  xbt_set_cursor_t cursor=NULL;
  my_elem_t         elm=NULL;

  xbt_set_foreach(set,cursor,elm) {
    xbt_assert0(elm,"Dude ! Got a null elm during traversal!");
    printf("   - Id(%d):  %s->%s\n",elm->ID,elm->name,elm->data);
    xbt_assert2(!strcmp(elm->name,elm->data),
		 "Key(%s) != value(%s). Abording",
		 elm->name,elm->data);
  }
}

static void search_not_found(xbt_set_t set, const char *data) {
  xbt_ex_t e;
  
  TRY {
    xbt_set_get_by_name(set,data);
    THROW1(unknown_error,0,"Found something which shouldn't be there (%s)",data);
  } CATCH(e) {
    if (e.category == mismatch_error) {
      xbt_ex_free(e);
    } else {
      RETHROW;
    }
  }
}

int main(int argc,char **argv) {
  xbt_set_t set=NULL;
  my_elem_t  elm;

  xbt_init(&argc,argv);
   
  printf("\nData set: USAGE test:\n");

  printf(" Traverse the empty set\n");
  traverse(set);

  fill(&set);
  printf(" Free the data set\n");
  xbt_set_free(&set);
  printf(" Free the data set again\n");
  xbt_set_free(&set);
  
  fill(&set);

  printf(" - Change some values\n");
  printf("   - Change 123 to 'Changed 123'\n");
  debuged_add_with_data(set,"123","Changed 123");
  printf("   - Change 123 back to '123'\n");
  debuged_add_with_data(set,"123","123");
  printf("   - Change 12a to 'Dummy 12a'\n");
  debuged_add_with_data(set,"12a","Dummy 12a");
  printf("   - Change 12a to '12a'\n");
  debuged_add_with_data(set,"12a","12a");

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */
  printf(" - Traverse the resulting data set\n");
  traverse(set);

  printf(" - Retrive values\n");
  elm = (my_elem_t) xbt_set_get_by_name(set,"123");
  xbt_assert(elm);
  strcmp("123",elm->data);

  search_not_found(set,"Can't be found");
  search_not_found(set,"123 Can't be found");
  search_not_found(set,"12345678 NOT");

  search_name(set,"12");
  search_name(set,"12a");
  search_name(set,"12b");
  search_name(set,"123");
  search_name(set,"123456");
  search_name(set,"1234");
  search_name(set,"123457");

  search_id(set,0,"12");
  search_id(set,1,"12a");
  search_id(set,2,"12b");
  search_id(set,3,"123");
  search_id(set,4,"123456");
  search_id(set,5,"1234");
  search_id(set,6,"123457");

  printf(" - Traverse the resulting data set\n");
  traverse(set);

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */

  printf(" Free the data set (twice)\n");
  xbt_set_free(&set);
  xbt_set_free(&set);

  printf(" - Traverse the resulting data set\n");
  traverse(set);

  xbt_exit();
  return 0;
}
