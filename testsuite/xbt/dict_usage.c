/* $Id$ */

/* dict_usage - A test of normal usage of a dictionnary                     */

/* Copyright (c) 2003,2004 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>

#include "gras.h"
#include "portable.h"
#include "xbt/ex.h"

XBT_LOG_EXTERNAL_CATEGORY(dict);
XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");

static void fill(xbt_dict_t *head);
static void debuged_add(xbt_dict_t head,const char*key);
static void search(xbt_dict_t head,const char*key);
static void debuged_remove(xbt_dict_t head,const char*key);
static void traverse(xbt_dict_t head);

static void print_str(void *str);
static void print_str(void *str) {
  printf("%s",(char*)PRINTF_STR(str));
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

  printf("   - Add %s\n",PRINTF_STR(key));
  xbt_dict_set(head,key,data,&free);
  if (XBT_LOG_ISENABLED(dict,xbt_log_priority_debug)) {
    xbt_dict_dump(head,(void (*)(void*))&printf);
    fflush(stdout);
  }
}

static void search(xbt_dict_t head,const char*key) {
  void *data;
  
  data=xbt_dict_get(head,key);
  printf("   - Search %s. Found %s\n",PRINTF_STR(key),(char*) PRINTF_STR(data));fflush(stdout);
  if (data && strcmp((char*)data,key)) 
    THROW2(mismatch_error,0,"Found %s while looking for %s",(char*)data,key);
}

static void debuged_remove(xbt_dict_t head,const char*key) {

  printf("   Remove '%s'\n",PRINTF_STR(key));fflush(stdout);
  xbt_dict_remove(head,key);
  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */
}


static void traverse(xbt_dict_t head) {
  xbt_dict_cursor_t cursor=NULL;
  char *key;
  char *data;

  xbt_dict_foreach(head,cursor,key,data) {
    printf("   - Seen:  %s->%s\n",PRINTF_STR(key),PRINTF_STR(data));
    xbt_assert2(!data || !strcmp(key,data),
		 "Key(%s) != value(%s). Abording\n",key,data);
  }
}

static void search_not_found(xbt_dict_t head, const char *data) {
  xbt_ex_t e;

  TRY {    
    data = xbt_dict_get(head,"Can't be found");
    THROW1(unknown_error,0,"Found something which shouldn't be there (%s)",data);
  } CATCH(e) {
    if (e.category != not_found_error) 
      RETHROW;
    xbt_ex_free(e);
  }
}

int main(int argc,char **argv) {
  xbt_ex_t e;
  xbt_dict_t head=NULL;
  char *data;

  xbt_init(&argc,argv);
   
  printf("\nGeneric dictionnary: USAGE test:\n");

  printf(" Traverse the empty dictionnary\n");
  traverse(head);

  printf(" Traverse the full dictionnary\n");
  fill(&head);
  search(head,"12a");
xbt_dict_dump(head,(void (*)(void*))&printf);
  traverse(head);

  printf(" Free the dictionnary (twice)\n");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
  
  fill(&head);

  /* xbt_dict_dump(head,(void (*)(void*))&printf);*/
  printf(" - Test that it works with NULL data\n");
  printf("   - Store NULL under 'null'\n");fflush(stdout);
  xbt_dict_set(head,"null",NULL,NULL);
  search(head,"null");
  /* xbt_dict_dump(head,(void (*)(void*))&printf); */
  printf("   Check whether I see it while traversing...\n");fflush(stdout);
  {
     xbt_dict_cursor_t cursor=NULL;
     char *key;
     int found=0;
     
     xbt_dict_foreach(head,cursor,key,data) {
	printf("   - Seen:  %s->%s\n",PRINTF_STR(key),PRINTF_STR(data));fflush(stdout);
	if (!strcmp(key,"null"))
	  found = 1;
     }
     xbt_assert0(found,"the key 'null', associated to NULL is not found");
  }
  printf("   OK, I did found the searched NULL\n");

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
  traverse(head);

  printf(" - Retrive values\n");
  data = xbt_dict_get(head,"123");
  xbt_assert(data);
  strcmp("123",data);

  search_not_found(head,"Can't be found");
  search_not_found(head,"123 Can't be found");
  search_not_found(head,"12345678 NOT");

  search(head,"12a");
  search(head,"12b");
  search(head,"12");
  search(head,"123456");
  search(head,"1234");
  search(head,"123457");

  printf(" - Traverse the resulting dictionnary\n");
  traverse(head);

  /*  xbt_dict_dump(head,(void (*)(void*))&printf); */

  printf(" Free the dictionnary twice\n");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  printf(" - Traverse the resulting dictionnary\n");
  traverse(head);

  printf("\n");
  fill(&head);
  printf(" - Remove the data (traversing the resulting dictionnary each time)\n");
  TRY {
    debuged_remove(head,"Does not exist");
  } CATCH(e) {
    if (e.category != not_found_error) 
      RETHROW;
    xbt_ex_free(e);
  }
  traverse(head);

  xbt_dict_free(&head);

  printf(" - Remove data from the NULL dict (error message expected)\n");
  TRY {
    debuged_remove(head,"12345");
  } CATCH(e) {
    if (e.category != arg_error) 
      RETHROW;
    xbt_ex_free(e);
  } 

  printf(" - Remove each data manually (traversing the resulting dictionnary each time)\n");
  fill(&head);
  debuged_remove(head,"12a");    traverse(head);
  debuged_remove(head,"12b");    traverse(head);
  debuged_remove(head,"12");     traverse(head);
  debuged_remove(head,"123456"); traverse(head);
  TRY {
    debuged_remove(head,"12346");
  } CATCH(e) {
    if (e.category != not_found_error) 
      RETHROW;
    xbt_ex_free(e);              traverse(head);
  } 
  debuged_remove(head,"1234");   traverse(head);
  debuged_remove(head,"123457"); traverse(head);
  debuged_remove(head,"123");    traverse(head);
  TRY {
    debuged_remove(head,"12346");
  } CATCH(e) {
    if (e.category != not_found_error) 
      RETHROW;
    xbt_ex_free(e);
  }                              traverse(head);
  
  printf(" - Free the dictionnary twice\n");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
  xbt_exit();
  return 0;
}
