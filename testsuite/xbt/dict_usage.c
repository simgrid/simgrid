/* $Id$ */

/* dict_usage - A test of normal usage of a dictionnary                     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <assert.h>

#include <gras.h>

GRAS_LOG_EXTERNAL_CATEGORY(dict);

static gras_error_t fill(gras_dict_t **head);
static gras_error_t debuged_add(gras_dict_t *head,const char*key);
static gras_error_t search(gras_dict_t *head,const char*key);
static gras_error_t debuged_remove(gras_dict_t *head,const char*key);
static gras_error_t traverse(gras_dict_t *head);

static void print_str(void *str);
static void print_str(void *str) {
  printf("%s",(char*)str);
}

static gras_error_t fill(gras_dict_t **head) {
  gras_error_t errcode;
  printf("\n Fill in the dictionnary\n");

  TRY(gras_dict_new(head));
  TRY(debuged_add(*head,"12"));
  TRY(debuged_add(*head,"12a"));
  TRY(debuged_add(*head,"12b"));
  TRY(debuged_add(*head,"123"));
  TRY(debuged_add(*head,"123456"));
  // Child becomes child of what to add
  TRY(debuged_add(*head,"1234"));
  // Need of common ancestor
  TRY(debuged_add(*head,"123457"));

  return no_error;
}

static gras_error_t debuged_add(gras_dict_t *head,const char*key)
{
  gras_error_t errcode;
  char *data=strdup(key);

  printf("   - Add %s\n",key);
  errcode=gras_dict_insert(head,key,data,&free);
  if (GRAS_LOG_ISENABLED(dict,gras_log_priority_debug)) {
    gras_dict_dump(head,(void (*)(void*))&printf);
    fflush(stdout);
  }
  return errcode;
}

static gras_error_t search(gras_dict_t *head,const char*key) {
  void *data;
  gras_error_t errcode;

  
  errcode=gras_dict_retrieve(head,key,&data);
  printf("   - Search %s. Found %s\n",key,data?(char*)data:"(null)");fflush(stdout);
  if (strcmp((char*)data,key)) 
    return mismatch_error;
  return errcode;
}

static gras_error_t debuged_remove(gras_dict_t *head,const char*key)
{
  gras_error_t errcode;

  printf("   Remove '%s'\n",key);fflush(stdout);
  errcode=gras_dict_remove(head,key);
  //  gras_dict_dump(head,(void (*)(void*))&printf);
  return errcode;
}


static gras_error_t traverse(gras_dict_t *head) {
  gras_dict_cursor_t *cursor=NULL;
  char *key;
  char *data;

  gras_dict_foreach(head,cursor,key,data) {
    printf("   - Seen:  %s->%s\n",key,data);
    if (strcmp(key,data)) {
      printf("Key(%s) != value(%s). Abording\n",key,data);
      abort();
    }
  }
  return no_error;
}

int main() {
  gras_error_t errcode;
  gras_dict_t *head=NULL;
  char *data;

  //  TRY(gras_log_control_set("root.thresh=info dict_collapse.thresh=debug"));
  //TRY(gras_log_control_set("root.thresh=info"));
  //  TRY(gras_log_control_set("root.thresh=info dict_search.thresh=info dict.thresh=debug dict_collapse.thresh=debug log.thresh=debug"));

  printf("\nGeneric dictionnary: USAGE test:\n");

  printf(" Traverse the empty dictionnary\n");
  TRYFAIL(traverse(head));

  TRYFAIL(fill(&head));
  printf(" Free the dictionnary\n");
  gras_dict_free(&head);

  TRYFAIL(fill(&head));

  printf(" - Change some values\n");
  printf("   - Change 123 to 'Changed 123'\n");
  TRYFAIL(gras_dict_insert(head,"123",strdup("Changed 123"),&free));
  printf("   - Change 123 back to '123'\n");
  TRYFAIL(gras_dict_insert(head,"123",strdup("123"),&free));
  printf("   - Change 12a to 'Dummy 12a'\n");
  TRYFAIL(gras_dict_insert(head,"12a",strdup("Dummy 12a"),&free));
  printf("   - Change 12a to '12a'\n");
  TRYFAIL(gras_dict_insert(head,"12a",strdup("12a"),&free));

  //  gras_dict_dump(head,(void (*)(void*))&printf);
  printf(" - Traverse the resulting dictionnary\n");
  TRYFAIL(traverse(head));

  printf(" - Retrive values\n");
  TRYFAIL(gras_dict_retrieve(head,"123",(void**)&data));
  assert(data);
  TRYFAIL(strcmp("123",data));

  TRYEXPECT(gras_dict_retrieve(head,"Can't be found",(void**)&data),mismatch_error);
  TRYEXPECT(gras_dict_retrieve(head,"123 Can't be found",(void**)&data),mismatch_error);
  TRYEXPECT(gras_dict_retrieve(head,"12345678 NOT",(void**)&data),mismatch_error);

  TRYFAIL(search(head,"12a"));
  TRYFAIL(search(head,"12b"));
  TRYFAIL(search(head,"12"));
  TRYFAIL(search(head,"123456"));
  TRYFAIL(search(head,"1234"));
  TRYFAIL(search(head,"123457"));

  printf(" - Traverse the resulting dictionnary\n");
  TRYFAIL(traverse(head));

  //  gras_dict_dump(head,(void (*)(void*))&printf);

  printf(" Free the dictionnary (twice)\n");
  gras_dict_free(&head);
  gras_dict_free(&head); // frees it twice to see if it triggers an error

  printf(" - Traverse the resulting dictionnary\n");
  TRYFAIL(traverse(head));

  printf("\n");
  TRYFAIL(fill(&head));
  printf(" - Remove the data (traversing the resulting dictionnary each time)\n");
  TRYEXPECT(debuged_remove(head,"Does not exist"),mismatch_error);
  TRYFAIL(traverse(head));

  TRYCATCH(debuged_remove(head,"12345"),mismatch_error);
  TRYFAIL(traverse(head));

  TRYFAIL(debuged_remove(head,"12a"));    TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"12b"));    TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"12"));     TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"123456")); TRYFAIL(traverse(head));
  TRYEXPECT(debuged_remove(head,"12346"),mismatch_error);  TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"1234"));   TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"123457")); TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"123"));    TRYFAIL(traverse(head));
  TRYEXPECT(debuged_remove(head,"12346"),mismatch_error);  TRYFAIL(traverse(head));
  
  gras_dict_free(&head);
  gras_dict_free(&head);

  return 0;
}
