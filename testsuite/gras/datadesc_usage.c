/* $Id$ */

/* datadesc: test of data description (using file transport).               */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <gras.h>

#include "../DataDesc/datadesc_interface.h"
GRAS_LOG_NEW_DEFAULT_CATEGORY(test);

gras_error_t
write_read(gras_datadesc_type_t *type,void *src, void *dst);

gras_error_t
write_read(gras_datadesc_type_t *type,void *src, void *dst) {
  gras_error_t errcode;
  gras_socket_t *sock;
   
  /* write */
  TRY(gras_socket_client_from_file("datadesc_usage.out",&sock));
  TRY(gras_datadesc_send(sock, type, src));
  gras_socket_close(&sock);
   
  /* read */
  TRY(gras_socket_server_from_file("datadesc_usage.out",&sock));
  TRY(gras_datadesc_recv(sock, type, gras_arch_selfid(), dst));
  gras_socket_close(&sock);
  
  return no_error;
}

gras_error_t test_int(void);
gras_error_t test_float(void);
gras_error_t test_array(void);
gras_error_t test_intref(void);
gras_error_t test_string(void);

gras_error_t test_homostruct(void);
gras_error_t test_hetestruct(void);
gras_error_t test_nestedstruct(void);
gras_error_t test_chain_list(void);
gras_error_t test_graph(void);

gras_error_t test_int(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *type;
  int i=5,*j=NULL;
  
  INFO0("==== Test on integer ====");
  TRY(gras_datadesc_by_name("int", &type));
  TRY(write_read(type, (void*)&i,(void**) &j));
  gras_assert(i == *j);
  free(j);
  return no_error;
}
gras_error_t test_float(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *type;
  float i=5.0,*j=NULL;
  
  INFO0("==== Test on float ====");
  TRY(gras_datadesc_by_name("float", &type));
  TRY(write_read(type, (void*)&i,(void**) &j));
  gras_assert(i == *j);
  free(j);
  return no_error;
}

#define SIZE 5
typedef int array[SIZE];
gras_error_t test_array(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *int_type;
  gras_datadesc_type_t *my_type;
  
  array i,*j;
  long int code;
  int cpt;

  INFO0("==== Test on fixed array ====");
  for (cpt=0; cpt<SIZE; cpt++) {
    i[cpt] = rand();
  }
  j=NULL;

  TRY(gras_datadesc_by_name("int", &int_type));
  TRY(gras_datadesc_declare_array("fixed array of int", int_type, 5, &code));
  TRY(gras_datadesc_by_code(code,&my_type));

  TRY(write_read(my_type, (void*)&i,(void**) &j));
  for (cpt=0; cpt<SIZE; cpt++)
    gras_assert(i[cpt] == (*j)[cpt]);
  free(j);
  return no_error;
}
gras_error_t test_intref(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *int_type;
  gras_datadesc_type_t *my_type;
  int *i,**j=NULL;
  long int code;
  
  if (! (i=malloc(sizeof(int))) )
    RAISE_MALLOC;
  *i=45;
  INFO1("==== Test on a reference to an integer (%p) ====",i);

  TRY(gras_datadesc_by_name("int", &int_type));
  TRY(gras_datadesc_declare_ref("int*",int_type,&code));
  TRY(gras_datadesc_by_code(code,&my_type));

  TRY(write_read(my_type, (void*)&i,(void**) &j));
  gras_assert(*i == **j);
  free(j);
  return no_error;
}

/***
 *** string (dynamic array)
 ***/ 
typedef char *string;
gras_error_t test_string(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *type;
  string i=strdup("Some data");
  string *j=NULL;
  
  INFO0("==== Test on string (dynamic array) ====");
  TRY(gras_datadesc_by_name("string", &type));
  TRY(write_read(type, (void*)&i,(void**) &j));
  gras_assert(!strcmp(i,*j));
  free(*j);
  return no_error;
}


/***
 *** homogeneous struct
 ***/ 
typedef struct {
  int a,b,c,d;
} homostruct;
gras_error_t test_homostruct(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  long int my_code;
  homostruct *i, *j; 

  INFO0("==== Test on homogeneous structure ====");
  /* create descriptor */
  TRY(gras_datadesc_declare_struct("homostruct",&my_code));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"a","int"));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"b","int"));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"c","int"));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"d","int"));
  TRY(gras_datadesc_by_code(my_code, &my_type));

  /* init a value, exchange it and check its validity*/
  if (! (i=malloc(sizeof(homostruct))) )
    RAISE_MALLOC;
  i->a = rand();  i->b = rand();
  i->c = rand();  i->d = rand();
  j = NULL;

  TRY(write_read(my_type, (void*)i, (void**)&j));
  gras_assert(i->a == j->a);
  gras_assert(i->b == j->b);
  gras_assert(i->c == j->c);
  gras_assert(i->d == j->d);

  free(i);
  free(j);
  return no_error;
}

/***
 *** heterogeneous struct
 ***/ 
typedef struct {
  unsigned char c1;
  unsigned long int l1;
  unsigned char c2;
  unsigned long int l2;
} hetestruct;
gras_error_t test_hetestruct(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  long int my_code;
  hetestruct *i, *j; 

  INFO0("==== Test on heterogeneous structure ====");
  /* create descriptor */
  TRY(gras_datadesc_declare_struct("hetestruct",&my_code));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"c1","unsigned char"));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"l1","unsigned long int"));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"c2","unsigned char"));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"l2","unsigned long int"));
  TRY(gras_datadesc_by_code(my_code, &my_type));

  /* init a value, exchange it and check its validity*/
  if (! (i=malloc(sizeof(hetestruct))) )
    RAISE_MALLOC;
  i->c1 = 's'; i->l1 = 123455;
  i->c2 = 'e'; i->l2 = 774531;
  j = NULL;

  TRY(write_read(my_type, (void*)i, (void**)&j));
  gras_assert(i->c1 == j->c1);
  gras_assert(i->c2 == j->c2);
  gras_assert(i->l1 == j->l1);
  gras_assert(i->l2 == j->l2);

  free(i);
  free(j);
  return no_error;
}

/***
 *** nested struct
 ***/ 
typedef struct {
  hetestruct hete;
  homostruct homo;
} nestedstruct;
gras_error_t test_nestedstruct(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  long int my_code;
  nestedstruct *i, *j; 

  INFO0("==== Test on nested structures ====");
  /* create descriptor */
  TRY(gras_datadesc_declare_struct("nestedstruct",&my_code));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"hete","hetestruct"));
  TRY(gras_datadesc_declare_struct_add_name(my_code,"homo","homostruct"));
  TRY(gras_datadesc_by_code(my_code, &my_type));

  /* init a value, exchange it and check its validity*/
  if (! (i=malloc(sizeof(nestedstruct))) )
    RAISE_MALLOC;
  i->homo.a = rand();  i->homo.b = rand();
  i->homo.c = rand();  i->homo.d = rand();
  i->hete.c1 = 's'; i->hete.l1 = 123455;
  i->hete.c2 = 'e'; i->hete.l2 = 774531;
  j = NULL;

  TRY(write_read(my_type, (void*)i, (void**)&j));
  gras_assert(i->homo.a == j->homo.a);
  gras_assert(i->homo.b == j->homo.b);
  gras_assert(i->homo.c == j->homo.c);
  gras_assert(i->homo.d == j->homo.d);
  gras_assert(i->hete.c1 == j->hete.c1);
  gras_assert(i->hete.c2 == j->hete.c2);
  gras_assert(i->hete.l1 == j->hete.l1);
  gras_assert(i->hete.l2 == j->hete.l2);

  free(i);
  free(j);
  return no_error;
}

/***
 *** chained list
 ***/ 
typedef struct s_chained_list chained_list_t;
struct s_chained_list {
  int          v;
  chained_list_t *l;
};
chained_list_t *cons(int v, chained_list_t *l);
void list_free(chained_list_t *l);
int list_eq(chained_list_t*i,chained_list_t*j);
chained_list_t * cons(int v, chained_list_t *l) {
  chained_list_t *nl = malloc(sizeof (chained_list_t));
  
  nl->v = v;
  nl->l = l;
  
  return nl;
}
void list_free(chained_list_t*l) {
  if (l) {
    list_free(l->l);
    free(l);
  }
}
int list_eq(chained_list_t*i,chained_list_t*j) {
  if (!i || !j) return i == j;
  if (i->v != j->v)
    return 0;
  return list_eq(i->l, j->l); 
}
gras_error_t test_chain_list(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type,*ref_my_type;
  long int my_code;
  long int ref_my_code;
  chained_list_t *i, *j; 

  INFO0("==== Test on chained list ====");
  /* create descriptor */
  TRY(gras_datadesc_declare_struct("chained_list_t",&my_code));
  TRY(gras_datadesc_by_code(my_code, &my_type));

  TRY(gras_datadesc_declare_ref("chained_list_t*",my_type,&ref_my_code));

  TRY(gras_datadesc_declare_struct_add_name(my_code,"v","int"));
  TRY(gras_datadesc_declare_struct_add_code(my_code,"l",ref_my_code));

  TRY(gras_datadesc_by_code(ref_my_code, &ref_my_type));
      
  /* init a value, exchange it and check its validity*/
  i = cons( rand(), cons( rand() , cons( rand(), NULL)));
  j = NULL;

  TRY(write_read(my_type, (void*)i, (void**)&j));
  gras_assert(list_eq(i,j));

  list_free(i);
  list_free(j);
  return no_error;
}
/***
 *** graph
 ***/
gras_error_t test_graph(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  chained_list_t *i, *j; 

  INFO0("==== Test on graph (cyclique chained list) ====");
  TRY(gras_datadesc_by_name("chained_list_t*", &my_type));
      
  /* init a value, exchange it and check its validity*/
  i = cons( rand(), cons( rand() , cons( rand(), NULL)));
  i->l->l->l = i->l;
  j = NULL;

  TRY(write_read(my_type, (void*)&i, (void**)&j));
  INFO1("j=%p"         ,j);
  INFO1("j->l=%p"      ,j->l);
  INFO1("j->l->l=%p"   ,j->l->l);
  INFO1("j->l->l->l=%p",j->l->l->l);
  gras_assert4(j->l->l->l == j->l,
	       "Received list is not cyclic. j->l=%p != j->l->l->l=%p\n"
	       "j=%p; &j=%p",
	       j->l,j->l->l->l, 
	       j ,&j);
  i->l->l->l = NULL;
  j->l->l->l = NULL;
  gras_assert(list_eq(i,j));

  list_free(i);
  list_free(j);
  return no_error;
}

/**** PBIO *****/
typedef struct { /* structure presented in the IEEE article */
  int Cnstatv;
  double Cstatev[12];
  int Cnprops;
  double Cprops[110];
  int Cndi[4], Cnshr, Cnpt;
  double Cdtime, Ctime[2];
  int Cntens;
  double Cdfgrd0[3][373], Cdfgrd1[3][3], Cstress[106], Cddsdde[106][106];
} KSdata1;

int main(int argc,char *argv[]) {
  gras_error_t errcode;

  gras_init_defaultlog(argc,argv,
		       "DataDesc.thresh=verbose"
		       " test.thresh=debug"
		       //		       " set.thresh=debug"
		       );

  /*
  TRYFAIL(test_int());    
  TRYFAIL(test_float());  
  TRYFAIL(test_array());  
  TRYFAIL(test_intref()); 
  TRYFAIL(test_string()); 

  TRYFAIL(test_homostruct());
  TRYFAIL(test_hetestruct());
  TRYFAIL(test_nestedstruct());
  */
  TRYFAIL(test_chain_list());
  TRYFAIL(test_graph());

  return 0;
}
