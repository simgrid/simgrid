/* $Id$ */

/* dict_usage - A test of normal usage of a dictionnary                     */

/* Copyright (c) 2003,2004 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>

#include "gras.h"

XBT_LOG_EXTERNAL_CATEGORY(dict);
XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");

static void fill(xbt_dict_t *head);
static void debuged_add(xbt_dict_t head,const char*key);
static xbt_error_t search(xbt_dict_t head,const char*key);
static xbt_error_t debuged_remove(xbt_dict_t head,const char*key);
static xbt_error_t traverse(xbt_dict_t head);

static void print_str(void *str);
static void print_str(void *str) {
  printf("%s",(char*)str);
}

static void fill(xbt_dict_t *head) {
  printf("\n Fill in the dictionnary\n");

  *head = xbt_dict_new();
  debuged_add(*head,"12");
  debuged_add(*head,"12a");
  debuged_add(*head,"12b");
  debuged_add(*head,"123");
  debuged_add(*head,"123456");
  /* Child becomes child of what to add */
  debuged_add(*head,"1234");
  /* Need of common ancestor */
  debuged_add(*head,"123457");

}

static void debuged_add(xbt_dict_t head,const char*key)
{
  char *data=xbt_strdup(key);

  printf("   - Add %s\n",key);
  xbt_dict_set(head,key,data,&free);
  if (XBT_LOG_ISENABLED(dict,xbt_log_priority_debug)) {
    xbt_dict_dump(head,(void (*)(void*))&printf);
    fflush(stdout);
  }
}

static xbt_error_t search(xbt_dict_t head,const char*key) {
  void *data;
  xbt_error_t errcode;

  
  errcode=xbt_dict_get(head,key,&data);
  printf("   - Search %s. Found %s\n",key,data?(char*)data:"(null)");fflush(stdout);
  if (!data)
     return errcode;
  if (strcmp((char*)data,key)) 
    return mismatch_error;
  return errcode;
}

static xbt_error_t debuged_remove(xbt_dict_t head,const char*key)
{
  xbt_error_t errcode;

  printf("   Remove '%s'\n",key);fflush(stdout);
  errcode=xbt_dict_remove(head,key);
  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */
  return errcode;
}


static xbt_error_t traverse(xbt_dict_t head) {
  xbt_dict_cursor_t cursor=NULL;
  char *key;
  char *data;

  xbt_dict_foreach(head,cursor,key,data) {
    printf("   - Seen:  %s->%s\n",key,data);
    xbt_assert2(!strcmp(key,data),
		 "Key(%s) != value(%s). Abording\n",key,data);
  }
  return no_error;
}

int main(int argc,char **argv) {
  xbt_error_t errcode;
  xbt_dict_t head=NULL;
  char *data;

  xbt_init_defaultlog(&argc,argv,"dict.thresh=verbose");
   
  printf("\nGeneric dictionnary: USAGE test:\n");

  printf(" Traverse the empty dictionnary\n");
  TRYFAIL(traverse(head));

  fill(&head);
  printf(" Free the dictionnary (twice)\n");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
  
  fill(&head);

  printf(" - Test that it works with NULL data\n");
  printf("   - Store NULL under 'null'\n");
  xbt_dict_set(head,"null",NULL,NULL);
  TRYFAIL(search(head,"null"));
   
  printf(" - Change some values\n");
  printf("   - Change 123 to 'Changed 123'\n");
  xbt_dict_set(head,"123",strdup("Changed 123"),&free);
  printf("   - Change 123 back to '123'\n");
  xbt_dict_set(head,"123",strdup("123"),&free);
  printf("   - Change 12a to 'Dummy 12a'\n");
  xbt_dict_set(head,"12a",strdup("Dummy 12a"),&free);
  printf("   - Change 12a to '12a'\n");
  xbt_dict_set(head,"12a",strdup("12a"),&free);

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */
  printf(" - Traverse the resulting dictionnary\n");
  TRYFAIL(traverse(head));

  printf(" - Retrive values\n");
  TRYFAIL(xbt_dict_get(head,"123",(void**)&data));
  xbt_assert(data);
  TRYFAIL(strcmp("123",data));

  TRYEXPECT(xbt_dict_get(head,"Can't be found",(void**)&data),mismatch_error);
  TRYEXPECT(xbt_dict_get(head,"123 Can't be found",(void**)&data),mismatch_error);
  TRYEXPECT(xbt_dict_get(head,"12345678 NOT",(void**)&data),mismatch_error);

  TRYFAIL(search(head,"12a"));
  TRYFAIL(search(head,"12b"));
  TRYFAIL(search(head,"12"));
  TRYFAIL(search(head,"123456"));
  TRYFAIL(search(head,"1234"));
  TRYFAIL(search(head,"123457"));

  printf(" - Traverse the resulting dictionnary\n");
  TRYFAIL(traverse(head));

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */

  printf(" Free the dictionnary twice\n");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  printf(" - Traverse the resulting dictionnary\n");
  TRYFAIL(traverse(head));

  printf("\n");
  fill(&head);
  printf(" - Remove the data (traversing the resulting dictionnary each time)\n");
  TRYEXPECT(debuged_remove(head,"Does not exist"),mismatch_error);
  TRYFAIL(traverse(head));

  xbt_dict_free(&head);

  printf(" - Remove data from the NULL dict (error message expected)\n");
  TRYCATCH(debuged_remove(head,"12345"),mismatch_error);

  printf(" - Remove each data manually (traversing the resulting dictionnary each time)\n");
  fill(&head);
  TRYFAIL(debuged_remove(head,"12a"));    TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"12b"));    TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"12"));     TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"123456")); TRYFAIL(traverse(head));
  TRYEXPECT(debuged_remove(head,"12346"),mismatch_error);  TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"1234"));   TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"123457")); TRYFAIL(traverse(head));
  TRYFAIL(debuged_remove(head,"123"));    TRYFAIL(traverse(head));
  TRYEXPECT(debuged_remove(head,"12346"),mismatch_error);  TRYFAIL(traverse(head));
  
  printf(" - Free the dictionnary twice\n");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
  xbt_exit();
  return 0;
}
